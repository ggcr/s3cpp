#include <print>
#include <stack>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct XMLNode {
    const std::string tag;
    const std::string value;
};

class XMLParser {
public:
    // Finite State Machine (FSM) for parsing S3 valid XML
    // See: TODO(cristian)
    std::vector<XMLNode> parse(const std::string& xml) {
        auto xmlElements = std::vector<XMLNode>();
        auto sv = std::string_view { xml };

        // Setup the initial state for our FSM
        auto state = States::Start;

        // Setup buffers we will use
        std::string currentTag = "";
        std::string currentTagClose = "";
        std::string currentBody = "";
        auto tagStack = std::stack<std::string> {};

        for (char ch : sv) {
            auto prevState = state;
            switch (state) {
            case States::Start: {
                if (ch == '<')
                    state = States::TagName;
                break;
            }
            case States::TagName: {
                if (ch == ' ')
                    state = States::TagAttr;
                else if (ch == '>') {
                    state = States::Body;
                    tagStack.push(currentTag);
                    currentTag = "";
                } else {
                    currentTag.push_back(ch);
                }
                break;
            }
            case States::TagAttr: {
                if (ch == '>') {
                    state = States::Body;
                    tagStack.push(currentTag);
                    currentTag = "";
                }
                break;
            }
            case States::Body: {
                if (ch == '<') {
                    state = States::Tag;
                } else {
                    currentBody.push_back(ch);
                }
                break;
            }
            case States::Tag: {
                if (ch == '/') {
                    state = States::TagClose;
                    if (currentTagClose.size() == 0)
                        currentTagClose = tagStack.top();
                } else {
                    currentTag.push_back(ch);
                    state = States::TagName;
                }
                break;
            }
            case States::TagClose: {
                if (ch != currentTagClose[0]) {
                    throw std::runtime_error(std::format("Invalid closing tag encountered: {} for char {}", currentTagClose, ch));
                } else {
                    currentTagClose.erase(0, 1);
                    if (currentTagClose.size() == 0)
                        state = States::Emit;
                }
                break;
            }
            case States::Emit: {
                if (tagStack.size() == 0)
                    throw std::runtime_error("Tag stack is empty");

                std::string tagName = tagStack.top();

                // Note: For now we will only return the leaf nodes
                // aka, those nodes that have an actual value
                if (currentBody.size() != 0) {
                    xmlElements.push_back(XMLNode { tagName, currentBody });
                    std::println("[EMIT] Tag={}, Body={}", tagName, currentBody);
                }

                state = States::Body;

                // Cleanup
                tagStack.pop();
                currentBody = "";
                break;
            }
            default:
                throw std::runtime_error(std::format("Invalid state reached: {}", std::to_underlying(state)));
            }
        }
        if (currentTag.size() == 0 && currentTagClose.size() == 0 && currentBody.size() == 0 && tagStack.size() == 0)
            return xmlElements;
        else
            throw std::runtime_error("Something went wrong");
    }

private:
    enum class States : int {
        Start,
        TagName,
        TagAttr,
        Body,
        Tag,
        TagClose,
        Emit,
    };
};
