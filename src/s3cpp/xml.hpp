#include <charconv>
#include <format>
#include <stack>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// ListBucketResult
// https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListObjectsV2.html#API_ListObjectsV2_ResponseSyntax

struct Contents {
    std::string ChecksumAlgorithm;
    std::string ChecksumType;
    std::string ETag;
    std::string Key;
    std::string LastModified;
    struct Owner_ {
        std::string DisplayName;
        std::string ID;
    } Owner;
    struct RestoreStatus_ {
        bool IsRestoreInProgress;
        std::string RestoreExpiryDate;
    } RestoreStatus;
    int64_t Size;
    std::string StorageClass;
};

struct CommonPrefix {
    std::string Prefix;
};

struct ListBucketResult {
    bool IsTruncated;
    std::string Marker;
    std::string NextMarker;
    std::vector<Contents> Contents;
    std::string Name;
    std::string Prefix;
    std::string Delimiter;
    int MaxKeys;
    std::vector<CommonPrefix> CommonPrefixes;
    std::string EncodingType;
    int KeyCount;
    std::string ContinuationToken;
    std::string NextContinuationToken;
    std::string StartAfter;
};

// We will use a regular Key Value struct to represent the raw XML nodes
// TODO(cristian): Make private
struct XMLNode {
    const std::string tag;
    const std::string value;
};

class XMLParser {
public:
    // Finite State Machine (FSM) for parsing S3 valid XML
    // See the automata on #10 whiteboard: https://ggcr.github.io/whiteboards/
    std::vector<XMLNode> parse(const std::string& xml) {
        auto xmlElements = std::vector<XMLNode>();
        auto sv = std::string_view { xml };

        // Setup the initial state for our FSM
        auto state = States::Start;

        // Setup buffers we will use
        std::string currentTag = "";
        std::string currentTagClose = "";
        std::string currentBody = "";
        std::string currentPath = "";
        std::string currentEntity = "";
        auto tagStack = std::stack<std::string> {};

        for (char ch : sv) {
            auto prevState = state;
            switch (state) {
            case States::Start: {
                if (ch == '<')
                    state = States::Processing;
                break;
            }
            case States::Processing: {
                // processing instructions are always self-contained
                if (ch == '?')
                    state = States::Start;
                else {
                    state = States::TagName;
                    currentTag.push_back(ch);
                    if (currentPath.size() >= 2 && currentPath[currentPath.size() - 2] != '.') {
                        currentPath.push_back('.');
                    }
                    currentPath.push_back(ch);
                }
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
                    currentPath.push_back(ch);
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
                } else if (ch == '&') {
                    state = States::Entity;
                } else {
                    currentBody.push_back(ch);
                }
                break;
            }
            case States::Entity: {
                if (ch == ';') {
                    // Decode entity and append it to currentBody
                    state = States::Body;
                    currentBody.push_back(decodeXMLEntity(currentEntity));
                    currentEntity = "";
                } else {
                    currentEntity.push_back(ch);
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
                    currentPath.push_back('.');
                    currentPath.push_back(ch);
                    state = States::Processing;
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
                    xmlElements.push_back(XMLNode { currentPath, currentBody });
                    // std::println("[EMIT] Tag={}, Body={}", currentPath, currentBody);
                }

                state = States::Body;

                // Cleanup
                tagStack.pop();
                if (auto pos = currentPath.find_last_of('.'); pos != std::string::npos) {
                    currentPath.erase(pos, std::string::npos);
                }
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

    char decodeXMLEntity(const std::string& entity) {
        // XML escape characters
        if (entity == "quot")
            return '"';
        else if (entity == "apos")
            return '\'';
        else if (entity == "lt")
            return '<';
        else if (entity == "gt")
            return '>';
        else if (entity == "amp")
            return '&';

        return parseNumber<char>(entity);

        throw std::runtime_error(std::format("Unknown XML entity: &{};", entity));
    }

    template <typename T>
    T parseNumber(const std::string s) {
        int code;
        std::from_chars_result result;
        int base = 10;

        // Parse XML numerical entities (i.e. '&#34;')
        if (s.starts_with('#') && s.size() > 1) {
            if (s[1] == 'x' || s[1] == 'X') {
                // Hex: #xhhhh
                base = 16;
                result = std::from_chars(s.data() + 2, s.data() + s.size(), code, base);
            } else {
                // Decimal: #hhhh
                base = 10;
                result = std::from_chars(s.data() + 1, s.data() + s.size(), code, base);
            }
        } else { // Regular case
            result = std::from_chars(s.data(), s.data() + s.size(), code, base);
        }

        if (result.ec == std::errc {}) {
            return static_cast<T>(code);
        }
        throw std::runtime_error(std::format("Unable to parse number from '{}'", s));
    }

    bool parseBool(const std::string& s) {
        if (s == "True" || s == "true")
            return true;
        else if (s == "False" || s == "false")
            return false;
        else
            throw std::runtime_error(std::format("Unable to parse boolean from string: '{}'", s));
    }

private:
    enum class States : int {
        Start,
        Processing,
        TagName,
        TagAttr,
        Body,
        Entity,
        Tag,
        TagClose,
        Emit,
    };
};
