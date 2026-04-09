#include <charconv>
#include <format>
#include <optional>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace s3cpp {
// We will use a regular Key Value struct to represent the raw XML nodes
struct XMLNode {
  const std::string tag;
  const std::string value;

  bool operator==(const XMLNode &other) const { return tag == other.tag && value == other.value; }
};

// New tree-based XML node with children and attributes
struct XMLNodeTree {
  std::string tag;
  std::string value;
  std::vector<XMLNodeTree> children;
  std::unordered_map<std::string, std::string> attributes;

  // Default constructor
  XMLNodeTree() = default;

  // Constructor with tag and value
  XMLNodeTree(std::string tag_, std::string value_)
      : tag(std::move(tag_)), value(std::move(value_)) {}

  // Equality operator
  bool operator==(const XMLNodeTree &other) const {
    return tag == other.tag && value == other.value && children == other.children &&
           attributes == other.attributes;
  }

  // Inequality operator
  bool operator!=(const XMLNodeTree &other) const { return !(*this == other); }

  // Returns const reference to children vector
  const std::vector<XMLNodeTree> &getChildren() const { return children; }

  // Returns const reference to attributes map
  const std::unordered_map<std::string, std::string> &getAttributes() const { return attributes; }

  // Check if node has children
  bool hasChildren() const { return !children.empty(); }

  // Check if node has attributes
  bool hasAttributes() const { return !attributes.empty(); }

  // Returns concatenated text content from node and all descendants
  std::string text() const {
    std::string result;
    if (!value.empty()) {
      result = value;
    }
    for (const auto &child : children) {
      result += child.text();
    }
    return result;
  }
};

// XMLTree container class for holding parsed XML structure
class XMLTree {
public:
  // Default constructor
  XMLTree() = default;

  // Constructor accepting raw XML string (placeholder for future parsing)
  explicit XMLTree(const std::string &xml) : root_{} {
    (void)xml;
  }

  // Constructor accepting XMLNodeTree
  explicit XMLTree(XMLNodeTree root) : root_(std::move(root)) {}

  // Get reference to root node
  XMLNodeTree &root() { return root_; }

  // Get const reference to root node
  const XMLNodeTree &root() const { return root_; }

  // Get size (number of nodes in tree)
  std::size_t size() const { return countNodes(root_); }

  // Check if tree is empty
  bool empty() const { return root_.tag.empty() && root_.value.empty() && root_.children.empty(); }

  // Find a node by dot-separated path (e.g., "Root.Child")
  // Returns pointer to node if found, nullptr otherwise
  XMLNodeTree *find(const std::string &path) {
    if (path.empty() || root_.tag.empty()) {
      return nullptr;
    }

    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;
    while (std::getline(ss, part, '.')) {
      parts.push_back(part);
    }

    if (parts.empty()) {
      return nullptr;
    }

    // First part must match root tag
    if (parts[0] != root_.tag) {
      return nullptr;
    }

    // Traverse down the path
    XMLNodeTree *current = &root_;
    for (size_t i = 1; i < parts.size(); ++i) {
      bool found = false;
      for (auto &child : current->children) {
        if (child.tag == parts[i]) {
          current = &child;
          found = true;
          break;
        }
      }
      if (!found) {
        return nullptr;
      }
    }

    return current;
  }

  // Const version of find
  const XMLNodeTree *find(const std::string &path) const {
    if (path.empty() || root_.tag.empty()) {
      return nullptr;
    }

    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;
    while (std::getline(ss, part, '.')) {
      parts.push_back(part);
    }

    if (parts.empty()) {
      return nullptr;
    }

    // First part must match root tag
    if (parts[0] != root_.tag) {
      return nullptr;
    }

    // Traverse down the path
    const XMLNodeTree *current = &root_;
    for (size_t i = 1; i < parts.size(); ++i) {
      bool found = false;
      for (const auto &child : current->children) {
        if (child.tag == parts[i]) {
          current = &child;
          found = true;
          break;
        }
      }
      if (!found) {
        return nullptr;
      }
    }

    return current;
  }

  // Find all nodes matching a dot-separated path pattern (e.g., "Root.Child")
  // Returns all matching nodes
  std::vector<XMLNodeTree *> findAll(const std::string &path) {
    std::vector<XMLNodeTree *> results;

    if (path.empty() || root_.tag.empty()) {
      return results;
    }

    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;
    while (std::getline(ss, part, '.')) {
      parts.push_back(part);
    }

    if (parts.empty()) {
      return results;
    }

    // First part must match root tag
    if (parts[0] != root_.tag) {
      return results;
    }

    // Start traversal from root
    findAllHelper(root_, parts, 1, results);

    return results;
  }

  // Convert tree to flat vector of XMLNode (backward compatibility)
  // Matches the output format of XMLParser::parse() - dot-separated paths like "Root.Child"
  std::vector<XMLNode> toFlatVector() const {
    std::vector<XMLNode> result;
    if (root_.tag.empty()) {
      return result;
    }
    // Start traversal with root tag as initial path
    toFlatVectorHelper(root_, root_.tag, result);
    return result;
  }

  // Const version of findAll
  std::vector<const XMLNodeTree *> findAll(const std::string &path) const {
    std::vector<const XMLNodeTree *> results;

    if (path.empty() || root_.tag.empty()) {
      return results;
    }

    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;
    while (std::getline(ss, part, '.')) {
      parts.push_back(part);
    }

    if (parts.empty()) {
      return results;
    }

    // First part must match root tag
    if (parts[0] != root_.tag) {
      return results;
    }

    // Start traversal from root
    findAllHelperConst(root_, parts, 1, results);

    return results;
  }

private:
  XMLNodeTree root_;

  // Helper function to count nodes recursively
  static std::size_t countNodes(const XMLNodeTree &node) {
    std::size_t count = 1;
    for (const auto &child : node.children) {
      count += countNodes(child);
    }
    return count;
  }

  // Helper function for findAll - non-const version
  static void findAllHelper(XMLNodeTree &node, const std::vector<std::string> &parts, size_t index,
                            std::vector<XMLNodeTree *> &results) {
    if (index >= parts.size()) {
      // Reached the target - add this node
      results.push_back(&node);
      return;
    }

    const std::string &targetTag = parts[index];
    for (auto &child : node.children) {
      if (child.tag == targetTag) {
        findAllHelper(child, parts, index + 1, results);
      }
    }
  }

  // Helper function for findAll - const version
  static void findAllHelperConst(const XMLNodeTree &node, const std::vector<std::string> &parts, size_t index,
                                 std::vector<const XMLNodeTree *> &results) {
    if (index >= parts.size()) {
      // Reached the target - add this node
      results.push_back(&node);
      return;
    }

    const std::string &targetTag = parts[index];
    for (const auto &child : node.children) {
      if (child.tag == targetTag) {
        findAllHelperConst(child, parts, index + 1, results);
      }
    }
  }

  // Helper function for toFlatVector - recursively traverses tree building dot-separated paths
  static void toFlatVectorHelper(const XMLNodeTree &node, const std::string &currentPath,
                                 std::vector<XMLNode> &result) {
    // If this node has a value, add it to the result (leaf node)
    if (!node.value.empty()) {
      result.push_back(XMLNode{currentPath, node.value});
    }

    // Recursively process children
    for (const auto &child : node.children) {
      std::string childPath = currentPath + "." + child.tag;
      toFlatVectorHelper(child, childPath, result);
    }
  }
};

class XMLParser {
public:
  // Finite State Machine (FSM) for parsing S3 valid XML
  // See the automata on #10 whiteboard: https://ggcr.github.io/whiteboards/
  std::vector<XMLNode> parse(const std::string &xml) {
    auto xmlElements = std::vector<XMLNode>();
    auto sv = std::string_view{xml};

    // Setup the initial state for our FSM
    auto state = States::Start;

    // Setup buffers we will use
    std::string currentTag;
    std::string currentTagClose;
    std::string currentBody;
    std::string currentPath;
    std::string currentEntity;
    auto tagStack = std::stack<std::string>{};
    int tagCloseIdx = 0;

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
          currentTag += ch;
          if (currentPath.size() >= 2 && currentPath[currentPath.size() - 2] != '.') {
            currentPath += '.';
          }
          currentPath += ch;
        }
        break;
      }
      case States::TagName: {
        if (ch == ' ')
          state = States::TagAttr;
        else if (ch == '>') {
          state = States::Body;
          tagStack.push(currentTag);
          currentTag.clear();
        } else {
          currentTag += ch;
          currentPath += ch;
        }
        break;
      }
      case States::TagAttr: {
        if (ch == '>') {
          state = States::Body;
          tagStack.push(currentTag);
          currentTag.clear();
        }
        break;
      }
      case States::Body: {
        if (ch == '<') {
          state = States::Tag;
        } else if (ch == '&') {
          state = States::Entity;
        } else {
          // Ignore leading spaces in the Body
          if (ch == ' ' && currentBody.size() == 0)
            break;
          currentBody += ch;
        }
        break;
      }
      case States::Entity: {
        if (ch == ';') {
          // Decode entity and append it to currentBody
          state = States::Body;
          currentBody += decodeXMLEntity(currentEntity);
          currentEntity.clear();
        } else {
          currentEntity += ch;
        }
        break;
      }
      case States::Tag: {
        if (ch == '/') {
          state = States::TagClose;
          if (tagCloseIdx == 0)
            currentTagClose = tagStack.top();
        } else {
          currentTag += ch;
          currentPath += '.';
          currentPath += ch;
          state = States::Processing;
        }
        break;
      }
      case States::TagClose: {
        if (ch != currentTagClose[tagCloseIdx]) {
          throw std::runtime_error(
              std::format("Invalid closing tag encountered: {} for char {}", currentTagClose, ch));
        } else {
          // currentTagClose.erase(0, 1);
          tagCloseIdx++;
          if (tagCloseIdx == currentTagClose.size()) {
            state = States::Emit;
            tagCloseIdx = 0;
          }
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
          xmlElements.push_back(XMLNode{currentPath, currentBody});
          // std::println("[EMIT] Tag={}, Body={}", currentPath, currentBody);
        }

        state = States::Body;

        // Cleanup
        tagStack.pop();
        if (auto pos = currentPath.find_last_of('.'); pos != std::string::npos) {
          currentPath.erase(pos, std::string::npos);
        }
        currentBody.clear();
        currentTagClose.clear();
        break;
      }
      default:
        throw std::runtime_error(std::format("Invalid state reached: {}", std::to_underlying(state)));
      }
    }
    if (currentTag.size() == 0 && currentTagClose.size() == 0 && currentBody.size() == 0 &&
        tagStack.size() == 0)
      return xmlElements;
    else
      throw std::runtime_error("Something went wrong");
  }

  char decodeXMLEntity(const std::string &entity) {
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

  // Build tree structure from XML string
  // Reuses FSM logic from parse() but constructs hierarchical tree
  XMLTree parseTree(const std::string &xml) {
    auto sv = std::string_view{xml};

    // Setup the initial state for our FSM
    auto state = States::Start;

    // Setup buffers we will use
    std::string currentTag;
    std::string currentTagClose;
    std::string currentBody;
    std::string currentEntity;
    std::string currentAttrName;
    std::string currentAttrValue;

    // Stack of indices to track position in the tree (not copying nodes)
    std::vector<size_t> nodeStack;
    nodeStack.reserve(64);

    // Create root node - will be returned as XMLTree
    XMLNodeTree rootNode;

    // Track if we're parsing attributes
    bool parsingAttrName = true;
    bool tagHasSelfClose = false;

    // Temporary storage for attributes between space chars
    std::vector<std::pair<std::string, std::string>> pendingAttrs;
    pendingAttrs.reserve(16);

    int tagCloseIdx = 0;

    for (char ch : sv) {
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
          currentTag += ch;
          // Reset attribute tracking for new tag
          currentAttrName.clear();
          currentAttrValue.clear();
          parsingAttrName = true;
          tagHasSelfClose = false;
        }
        break;
      }
      case States::TagName: {
        if (ch == ' ') {
          state = States::TagAttr;
        } else if (ch == '/') {
          // Self-closing tag marker
          tagHasSelfClose = true;
        } else if (ch == '>') {
          state = States::Body;
          // Create new node
          XMLNodeTree newNode(currentTag, "");

          if (nodeStack.empty()) {
            // This is the root node
            rootNode = std::move(newNode);
            // Push special marker for root (-1) to indicate we're at root level
            nodeStack.push_back(static_cast<size_t>(-1));
          } else {
            // Add as child to parent (find parent by traversing from root)
            XMLNodeTree* parent = findNodeByStackIndex(rootNode, nodeStack);
            parent->children.push_back(std::move(newNode));
            // Push child index to stack (except for self-closing tags)
            if (!tagHasSelfClose) {
              nodeStack.push_back(parent->children.size() - 1);
            }
          }

          currentTag.clear();
        } else {
          currentTag += ch;
        }
        break;
      }
      case States::TagAttr: {
        if (ch == '>') {
          state = States::Body;
          // Create node with attributes
          XMLNodeTree newNode(currentTag, "");

          // Add any pending attributes to the new node
          if (!currentAttrName.empty() && !currentAttrValue.empty()) {
            newNode.attributes[currentAttrName] = currentAttrValue;
          }
          // Also add any attributes stored in pendingAttrs
          for (auto &attr : pendingAttrs) {
            newNode.attributes[attr.first] = attr.second;
          }
          pendingAttrs.clear();

          if (nodeStack.empty()) {
            // This is root node
            rootNode = std::move(newNode);
            // Push special marker for root (-1) to indicate we're at root level
            nodeStack.push_back(static_cast<size_t>(-1));
          } else {
            // Add as child to parent (find parent by traversing from root using stack)
            XMLNodeTree* parent = findNodeByStackIndex(rootNode, nodeStack);
            parent->children.push_back(std::move(newNode));
            if (!tagHasSelfClose) {
              nodeStack.push_back(parent->children.size() - 1);
            }
          }

          currentTag.clear();
        } else if (ch == '/') {
          tagHasSelfClose = true;
        } else if (ch == '=') {
          parsingAttrName = false;
        } else if (ch == '"') {
          // Skip quotes for now, value parsing
        } else if (ch == ' ') {
          // Space between attributes - save current attribute pair for later
          if (!currentAttrName.empty() && !currentAttrValue.empty()) {
            // Store attribute in temporary storage - we'll apply when node is created
            pendingAttrs.push_back({currentAttrName, currentAttrValue});
            currentAttrName.clear();
            currentAttrValue.clear();
            parsingAttrName = true;
          }
        } else {
          if (parsingAttrName) {
            currentAttrName += ch;
          } else {
            currentAttrValue += ch;
          }
        }
        break;
      }
      case States::Body: {
        if (ch == '<') {
          state = States::Tag;
        } else if (ch == '&') {
          state = States::Entity;
        } else {
          // Ignore leading spaces in the Body
          if (ch == ' ' && currentBody.size() == 0)
            break;
          currentBody += ch;
        }
        break;
      }
      case States::Entity: {
        if (ch == ';') {
          // Decode entity and append it to currentBody
          state = States::Body;
          currentBody += decodeXMLEntity(currentEntity);
          currentEntity.clear();
        } else {
          currentEntity += ch;
        }
        break;
      }
      case States::Tag: {
        if (ch == '/') {
          state = States::TagClose;
          if (tagCloseIdx == 0 && !nodeStack.empty()) {
            // Get tag name from root node by traversing stack
            XMLNodeTree* current = findNodeByStackIndex(rootNode, nodeStack);
            currentTagClose = current->tag;
          }
        } else {
          currentTag += ch;
          state = States::Processing;
        }
        break;
      }
      case States::TagClose: {
        if (ch != currentTagClose[tagCloseIdx]) {
          throw std::runtime_error(
              std::format("Invalid closing tag encountered: {} for char {}", currentTagClose, ch));
        } else {
          tagCloseIdx++;
          if (tagCloseIdx == currentTagClose.size()) {
            state = States::Emit;
            tagCloseIdx = 0;
          }
        }
        break;
      }
      case States::Emit: {
        if (nodeStack.size() == 0)
          throw std::runtime_error("Tag stack is empty");

        // Find the current node using the stack indices
        XMLNodeTree* current = findNodeByStackIndex(rootNode, nodeStack);

        // Set the body value to the current node
        if (currentBody.size() != 0) {
          current->value = currentBody;
        }

        // Pop current index from stack (we're done with this node)
        nodeStack.pop_back();

        state = States::Body;

        // Cleanup
        currentBody.clear();
        currentTagClose.clear();
        break;
      }
      default:
        throw std::runtime_error(std::format("Invalid state reached: {}", std::to_underlying(state)));
      }
    }

    // Finalize: set any remaining body content to current node
    if (currentBody.size() != 0 && !nodeStack.empty()) {
      XMLNodeTree* current = findNodeByStackIndex(rootNode, nodeStack);
      current->value = currentBody;
    }

    return XMLTree(rootNode);
  }

  // Helper function to find a node in the tree by traversing using stack indices
  // Special marker -1 (represented as max size_t) indicates root level
  static XMLNodeTree* findNodeByStackIndex(XMLNodeTree& root, const std::vector<size_t>& stack) {
    XMLNodeTree* current = &root;
    for (size_t idx : stack) {
      // Special marker: -1 (max size_t) indicates we want root level, skip it
      if (idx == static_cast<size_t>(-1)) {
        continue; // skip root marker, continue to process remaining indices
      }
      if (idx >= current->children.size()) {
        return nullptr;
      }
      current = &current->children[idx];
    }
    return current;
  }

  template <typename T> T parseNumber(const std::string s) {
    T code;
    int base = 10, offset = 0;

    // Parse XML numerical entities (i.e. '&#34;')
    if (s.starts_with('#') && s.size() > 1) {
      if (s[1] == 'x' || s[1] == 'X') {
        // Hex: #xhhhh
        base = 16;
        offset = 2;
      } else {
        // Decimal: #hhhh
        offset = 1;
      }
    }

    std::from_chars_result result = std::from_chars(s.data() + offset, s.data() + s.size(), code, base);

    if (result.ec == std::errc{}) {
      return code;
    }
    throw std::runtime_error(std::format("Unable to parse number from '{}'", s));
  }

  bool parseBool(const std::string &s) {
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
} // namespace s3cpp
