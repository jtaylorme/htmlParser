/*
 * Original Version Copyright (c) 2017 SPLI (rangerlee@foxmail.com)
 * Latest version available at: http://github.com/rangerlee/htmlparser.git
 *
 * A Simple html parser.
 * More information can get from README.md
 *
 * 
 * Changed Version Copyright (c) 2025 DSI (jtaylor@jtdata.com)
 * Latest version available at: https://github.com/jtaylorme/html_parser
 *
 * 
 */

#ifndef HTMLPARSER_HPP_
#define HTMLPARSER_HPP_

#include <string>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <memory>


using std::enable_shared_from_this;
using std::shared_ptr;
using std::weak_ptr;

static std::wstring toLowerW(const std::wstring& str);
static std::string toLower(const std::string& str);
inline std::string EscapeForXPath(const std::string& value);


/**
 * class HtmlElement
 * HTML Element struct
 */
class HtmlElement : public enable_shared_from_this<HtmlElement> {
public:
    friend class HtmlParser;

    friend class HtmlDocument;

public:
    /**
     * for children traversals.
     */
    typedef std::vector<shared_ptr<HtmlElement> >::const_iterator ChildIterator;

    const ChildIterator ChildBegin() {
        return children.begin();
    }

    const ChildIterator ChildEnd() {
        return children.end();
    }

    /**
     * for attribute traversals.
     */
    typedef std::map<std::string, std::string>::const_iterator AttributeIterator;

    const AttributeIterator AttributeBegin() {
        return attribute.begin();
    }

    const AttributeIterator AttributeEnd() {
        return attribute.end();
    }

public:

    HtmlElement() {}

    HtmlElement(shared_ptr<HtmlElement> p)
        : parent(p) {
    }

    std::string GetAttribute(const std::string& k) {
        if (attribute.find(k) != attribute.end()) {
            return attribute[k];
        }
        return "";
    }

    void SetAttribute(const std::string& j, const std::string& k) {
        if (k.empty()) {
            attribute.erase(j);
            if (j == "class") classlist.clear();
        }
        else {
            attribute[j] = k;
            if (j == "class") {
                classlist.clear();
                std::istringstream iss(k);
                std::string token;
                while (iss >> token) {
                    classlist.push_back(token);
                }
            }
        }
    }


    std::map<std::string, std::string> GetAttributes() {

        return attribute;

    }


    shared_ptr<HtmlElement> GetElementById(const std::string& id) {
        // MessageBox(NULL, id.c_str(), "YO", MB_OK);
        for (HtmlElement::ChildIterator it = children.begin(); it != children.end(); ++it) {
            if ((*it)->GetAttribute("id") == id)
            {
                //  MessageBox(NULL, id.c_str(), "YO2", MB_OK);
                return *it;
            }
            shared_ptr<HtmlElement> r = (*it)->GetElementById(id);

            if (r) return r;
        }

        return shared_ptr<HtmlElement>();
    }

    std::vector<shared_ptr<HtmlElement> > GetElementsById(const std::string& id) {
        std::vector<shared_ptr<HtmlElement> > result;
        GetElementsById(id, result);
        return result;
    }

    

    std::vector<shared_ptr<HtmlElement> > GetElementsByClassName(const std::string& name) {
        std::vector<shared_ptr<HtmlElement> > result;
        GetElementsByClassName(name, result);
        return result;
    }

    // Inside HtmlElement class (public:)
    std::vector<std::string> GetClassList() const {
        return classlist;
    }

    bool HasClass(const std::string& cls) const {
        return std::find(classlist.begin(), classlist.end(), cls) != classlist.end();
    }

    void AddClass(const std::string& cls) {
        if (!HasClass(cls)) {
            classlist.push_back(cls);
            UpdateClassAttribute();
        }
    }

    void RemoveClass(const std::string& cls) {
        auto it = std::remove(classlist.begin(), classlist.end(), cls);
        if (it != classlist.end()) {
            classlist.erase(it, classlist.end());
            UpdateClassAttribute();
        }
    }

    void ToggleClass(const std::string& cls) {
        if (HasClass(cls))
            RemoveClass(cls);
        else
            AddClass(cls);
    }

    void ClearClasses() {
        classlist.clear();
        attribute.erase("class");
    }


    std::vector<shared_ptr<HtmlElement> > GetElementByTagName(const std::string& name) {
        std::vector<shared_ptr<HtmlElement> > result;
        GetElementByTagName(name, result);
        return result;
    }

    // Enhanced SelectElement to support wildcard `*` and a few basic XPath-like improvements
    void SelectElement(const std::string& rule, std::vector<shared_ptr<HtmlElement> >& result) {
        if (rule.empty() || rule.at(0) != '/' || name == "plain") return;
        std::string::size_type pos = 0;
        if (rule.size() >= 2 && rule.at(1) == '/') {
            std::vector<shared_ptr<HtmlElement> > temp;
            GetAllElement(temp);
            pos = 1;
            std::string next = rule.substr(pos);
            if (next.empty()) {
                for (size_t i = 0; i < temp.size(); i++) {
                    InsertIfNotExists(result, temp[i]);
                }
            }
            else {
                for (size_t i = 0; i < temp.size(); i++) {
                    temp[i]->SelectElement(next, result);;
                }
            }
        }
        else {
            std::string::size_type p = rule.find('/', 1);
            std::string line;
            if (p == std::string::npos) {
                line = rule;
                pos = rule.size();
            }
            else {
                line = rule.substr(0, p);
                pos = p;
            }

            enum { x_ele, x_wait_attr, x_attr, x_val };
            std::string ele, attr, oper, val, cond;
            int state = x_ele;
            for (p = 1; p < pos; ) {
                char c = line.at(p++);
                switch (state) {
                case x_ele: {
                    if (c == '@') {
                        state = x_attr;
                    }
                    else if (c == '!') {
                        state = x_wait_attr;
                        cond.append(1, c);
                    }
                    else if (c == '[') {
                        state = x_wait_attr;
                    }
                    else {
                        ele.append(1, c);
                    }
                }
                          break;

                case x_wait_attr: {
                    if (c == '@') state = x_attr;
                    else if (c == '!') {
                        cond.append(1, c);
                    }
                }
                                break;

                case x_attr: {
                    if (c == '!') {
                        oper.append(1, c);
                    }
                    else if (c == '=') {
                        oper.append(1, c);
                        state = x_val;
                    }
                    else if (c == ']') {
                        state = x_ele;
                    }
                    else {
                        attr.append(1, c);
                    }
                }
                           break;

                case x_val: {
                    if (c == ']') {
                        state = x_ele;
                    }
                    else {
                        val.append(1, c);
                    }
                }
                          break;
                }
            }

            if (!val.empty() && val.at(0) == '\'') {
                val.erase(val.begin());
            }

            if (!val.empty() && val.at(val.size() - 1) == '\'') {
                val.pop_back();
            }

            bool matched = true;
            if (!ele.empty()) {
                if (toLower(name) != toLower(ele)) {
                    matched = false;
                }
            }

            if (cond == "!") {
                if (!attr.empty() && matched) {
                    if (!oper.empty()) {
                        std::string v = attribute[attr];
                        if (oper == "=") {
                            if (v == val) matched = false;
                            if (attr == "class") {
                                if (!HasClass(val)) matched = false;
                            }

                        }
                        else if (oper == "!=") {
                            if (v == val) matched = false;
                            if (attr == "class") {
                                if (HasClass(val)) matched = false;
                            }

                        }
                    }
                    else {
                        if (attribute.find(attr) != attribute.end()) matched = false;
                    }
                }
            }
            else {
                if (!attr.empty() && matched) {
                    if (attribute.find(attr) == attribute.end()) {
                        matched = false;
                    }
                    else {
                        std::string v = attribute[attr];
                        if (oper == "=") {
                            if (v != val) matched = false;
                            if (attr == "class") {
                                if (!HasClass(val)) matched = false;
                            }

                        }
                        else if (oper == "!=") {
                            if (v == val) matched = false;
                            if (attr == "class") {
                                if (HasClass(val)) matched = false;
                            }

                        }
                    }
                }
            }

            std::string next = rule.substr(pos);
            if (matched) {
                if (next.empty())
                    InsertIfNotExists(result, shared_from_this());
                else {
                    for (ChildIterator it = ChildBegin(); it != ChildEnd(); it++) {
                        (*it)->SelectElement(next, result);
                    }
                }
            }
        };
    }

    shared_ptr<HtmlElement> GetParent() {
        return parent.lock();
    }

    shared_ptr<HtmlElement> GetSiblingNext() {

        shared_ptr<HtmlElement> el = shared_from_this();

        if (el->GetParent()) {
            auto& children = el->GetParent()->children;

            auto it = std::find(children.begin(), children.end(), el);

            if (it != children.end()) {
                if (it + 1 != children.end()) {
                    auto nextSibling = *(it + 1);
                    return nextSibling;
                }
            }
        }
        return nullptr;
    }


    std::vector<shared_ptr<HtmlElement>> GetChildren() {

        return children;

    }

    shared_ptr<HtmlElement> GetSiblingPrev() {

        shared_ptr<HtmlElement> el = shared_from_this();

        if (el->GetParent()) {
            auto& children = el->GetParent()->children;

            auto it = std::find(children.begin(), children.end(), el);

            if (it != children.end()) {
                if (it != children.begin()) {
                    auto prevSibling = *(it - 1);
                    return prevSibling;
                }
            }
        }
        return nullptr;
    }



    int SetInnerText(std::string text) {
        auto el = shared_from_this();

        if (el->children.empty()) {
            // Create a text node if none exists
            auto textNode = std::make_shared<HtmlElement>();
            textNode->value = text;
            el->children.push_back(textNode);

        }
        else {
            el->children[0]->value = text; // safe now
        }

        return 0;
    }

    int SetInnerHTML(std::shared_ptr<HtmlElement> tempRoot) {
        auto el = shared_from_this();

        // Clear current children
        el->children.clear();

        // Append parsed children to our element
        for (auto& child : tempRoot->GetChildren()) {
            // Make a new HtmlElement with the same data but correct parent
            child->parent = el;
            el->children.push_back(child);
        }

        return 0;
    }


    const std::string& GetValue() {
        if (value.empty() && children.size() == 1 && children[0]->GetName() == "plain") {
            return children[0]->GetValue();
        }

        return value;
    }


    const std::string& GetName() {
        return name;
    }

    std::string text() {
        std::string str;
        PlainStylize(str);
        return str;
    }

    void PlainStylize(std::string& str) {
        if (name == "head" || name == "meta" || name == "style" || name == "script" || name == "link") {
            return;
        }

        if (name == "plain") {
            str.append(value);
            return;
        }

        for (size_t i = 0; i < children.size();) {
            children[i]->PlainStylize(str);

            if (++i < children.size()) {
                std::string ele = children[i]->GetName();
                if (ele == "td") {
                    str.append("\t");
                }
                else if (ele == "tr" || ele == "br" || ele == "div" || ele == "p" || ele == "hr" || ele == "area" ||
                    ele == "h1" || ele == "h2" || ele == "h3" || ele == "h4" || ele == "h5" || ele == "h6" || ele == "h7") {
                    str.append("\n");
                }
            }
        }
    }

    std::string OuterHTML() {
        std::string str;
        HtmlStylize(str);
        return str;
    }

    std::string InnerHTML() {
        std::string str;

        // Add inner text if there are no children
        if (children.empty()) {
            str.append(value);
        }
        else {
            for (size_t i = 0; i < children.size(); ++i) {
                children[i]->HtmlStylize(str);
            }
        }

        return str;
    }


    void HtmlStylize(std::string& str) {
        if (name.empty()) {
            for (size_t i = 0; i < children.size(); i++) {
                children[i]->HtmlStylize(str);
            }

            return;
        }
        else if (name == "plain") {
            str.append(value);
            return;
        }

        str.append("<" + name);
        std::map<std::string, std::string>::const_iterator it = attribute.begin();
        for (; it != attribute.end(); it++) {
            str.append(" " + it->first + "=\"" + it->second + "\"");
        }
        str.append(">");

        if (children.empty()) {
            str.append(value);
        }
        else {
            for (size_t i = 0; i < children.size(); i++) {
                children[i]->HtmlStylize(str);
            }
        }

        str.append("</" + name + ">");
    }

private:
   

    void GetElementsByClassName(const std::string& cls, std::vector<std::shared_ptr<HtmlElement>>& result)
    {

        if (HasClass(cls))
            InsertIfNotExists(result, shared_from_this());

        for (ChildIterator it = ChildBegin(); it != ChildEnd(); ++it) {
            (*it)->GetElementsByClassName(cls, result);
        }
    }


    void GetElementsById(const std::string& id, std::vector<shared_ptr<HtmlElement> >& result) {
        std::string xpath = "//*[@id='" + EscapeForXPath(id) + "']";
        //   if (id == "description") myMsg(9, id + " - " + xpath);
        SelectElement(xpath, result);
        // if (id == "description") myMsg(result.size(), id + " - " + xpath);
        return;
    }

    void GetElementByTagName(const std::string& name, std::vector<shared_ptr<HtmlElement>>& result) {
        for (HtmlElement::ChildIterator it = children.begin(); it != children.end(); ++it) {
            if (_stricmp((*it)->name.c_str(), name.c_str()) == 0)
                InsertIfNotExists(result, *it);

            (*it)->GetElementByTagName(name, result);
        }
    }


    void GetAllElement(std::vector<shared_ptr<HtmlElement> >& result) {
        for (size_t i = 0; i < children.size(); ++i) {
            InsertIfNotExists(result, children[i]);
            children[i]->GetAllElement(result);
        }
    }

    void Parse(const std::string& attr) {
        size_t index = 0;
        std::string k;
        std::string v;
        char split = ' ';
        bool quota = false;

        enum ParseAttrState {
            PARSE_ATTR_KEY,
            PARSE_ATTR_VALUE_BEGIN,
            PARSE_ATTR_VALUE_END,
        };

        ParseAttrState state = PARSE_ATTR_KEY;

        while (attr.size() > index) {
            char input = attr.at(index);
            switch (state) {
            case PARSE_ATTR_KEY: {
                if (input == '\t' || input == '\r' || input == '\n') {
                }
                else if (input == '\'' || input == '"') {
                    std::cerr << "WARN : attribute unexpected " << input << std::endl;
                }
                else if (input == ' ') {
                    if (!k.empty()) {
                        attribute[k] = v;
                        k.clear();
                    }
                }
                else if (input == '=') {
                    state = PARSE_ATTR_VALUE_BEGIN;
                }
                else {
                    k.append(attr.c_str() + index, 1);
                }
            }
                               break;

            case PARSE_ATTR_VALUE_BEGIN: {
                if (input == '\t' || input == '\r' || input == '\n' || input == ' ') {
                    if (!k.empty()) {
                        attribute[k] = v;
                        k.clear();
                    }
                    state = PARSE_ATTR_KEY;
                }
                else if (input == '\'' || input == '"') {
                    split = input;
                    quota = true;
                    state = PARSE_ATTR_VALUE_END;
                }
                else {
                    v.append(attr.c_str() + index, 1);
                    quota = false;
                    state = PARSE_ATTR_VALUE_END;
                }
            }
                                       break;

            case PARSE_ATTR_VALUE_END: {
                if ((quota && input == split) || (!quota && (input == '\t' || input == '\r' || input == '\n' || input == ' '))) {
                    attribute[k] = v;
                    k.clear();
                    v.clear();
                    state = PARSE_ATTR_KEY;
                }
                else {
                    v.append(attr.c_str() + index, 1);
                }
            }
                                     break;
            }

            index++;
        }

        if (!k.empty()) {
            attribute[k] = v;
        }

        //trim
        if (!value.empty()) {
            value.erase(0, value.find_first_not_of(" "));
            value.erase(value.find_last_not_of(" ") + 1);
        }

        // After parsing attributes into `attribute`
        auto it = attribute.find("class");
        if (it != attribute.end()) {
            classlist.clear();
            std::istringstream iss(it->second);
            std::string token;
            while (iss >> token) {
                classlist.push_back(token);
            }
        }

    }
 

    static void InsertIfNotExists(std::vector<std::shared_ptr<HtmlElement>>& vec, const std::shared_ptr<HtmlElement>& ele) {
        for (size_t i = 0; i < vec.size(); i++) {
            if (vec[i] == ele) return;
        }

        vec.push_back(ele);
    }

    // Private helper to sync classlist attribute["class"]
private:
    void UpdateClassAttribute() {
        if (classlist.empty()) {
            attribute.erase("class");
            return;
        }
        std::string combined;
        for (size_t i = 0; i < classlist.size(); ++i) {
            if (i > 0) combined += " ";
            combined += classlist[i];
        }
        attribute["class"] = combined;
    }
private:
    std::string name;
    std::string value;
    std::map<std::string, std::string> attribute;
    std::vector<std::string> classlist;
    weak_ptr<HtmlElement> parent;
    std::vector<shared_ptr<HtmlElement> > children;
};

/**
 * class HtmlDocument
 * Html Doc struct
 */
class HtmlDocument {
public:
    HtmlDocument(shared_ptr<HtmlElement>& root)
        : root_(root) {}

    std::shared_ptr<HtmlElement> GetRoot() {
        return root_;
    }
    shared_ptr<HtmlElement> GetElementById(const std::string& id) {
        return root_->GetElementById(id);
    }

    std::vector<shared_ptr<HtmlElement> >  GetElementsById(const std::string& id) {
        return root_->GetElementsById(id);
    }
    
    std::vector<shared_ptr<HtmlElement> > GetElementsByClassName(const std::string& name) {
        return root_->GetElementsByClassName(name);
    }
    std::vector<shared_ptr<HtmlElement> > GetElementByTagName(const std::string& name) {
        return root_->GetElementByTagName(name);
    }

    std::vector<shared_ptr<HtmlElement> > SelectElement(const std::string& rule, std::vector<shared_ptr<HtmlElement>>& result) {
        HtmlElement::ChildIterator it = root_->ChildBegin();
        for (; it != root_->ChildEnd(); it++) {
            (*it)->SelectElement(rule, result);
        }

        return result;
    }

    std::string OuterHTML() {
        return root_->OuterHTML();
    }
    std::string InnerHTML() {
        return InnerHTML();
    }
    std::string text() {
        return root_->text();
    }

private:
    shared_ptr<HtmlElement> root_;
};

/**
 * class HtmlParser
 * html parser and only one interface
 */
class HtmlParser {
public:
    HtmlParser() {
        static const std::string token[] = { "br", "hr", "img", "input", "link", "meta",
        "area", "base", "col", "command", "embed", "keygen", "param", "source", "track", "wbr" };
        self_closing_tags_.insert(token, token + sizeof(token) / sizeof(token[0]));
    }

    /**
     * parse html by C-Style data
     * @param data
     * @param len
     * @return html document object
     */
    shared_ptr<HtmlDocument> Parse(const char* data, size_t len) {
        stream_ = data;
        length_ = len;
        size_t index = 0;
        root_.reset(new HtmlElement());
        while (length_ > index) {
            char input = stream_[index];
            if (input == '\r' || input == '\n' || input == '\t' || input == ' ') {
                index++;
            }
            else if (input == '<') {
                index = ParseElement(index, root_);
            }
            else {
                break;
            }
        }

        return shared_ptr<HtmlDocument>(new HtmlDocument(root_));
    }

    /**
     * parse html by string data
     * @param data
     * @return html document object
     */
    shared_ptr<HtmlDocument> Parse(const std::string& data) {
        return Parse(data.data(), data.size());
    }

private:
    size_t ParseElement(size_t index, shared_ptr<HtmlElement>& element) {
        while (length_ > index) {
            char input = stream_[index + 1];
            if (input == '!') {
                if (strncmp(stream_ + index, "<!--", 4) == 0) {
                    return SkipUntil(index + 2, "-->");
                }
                else {
                    return SkipUntil(index + 2, '>');
                }
            }
            else if (input == '/') {
                return SkipUntil(index, '>');
            }
            else if (input == '?') {
                return SkipUntil(index, "?>");
            }

            shared_ptr<HtmlElement> self(new HtmlElement(element));

            enum ParseElementState {
                PARSE_ELEMENT_TAG,
                PARSE_ELEMENT_ATTR,
                PARSE_ELEMENT_VALUE,
                PARSE_ELEMENT_TAG_END
            };

            ParseElementState state = PARSE_ELEMENT_TAG;
            index++;
            char split = 0;
            std::string attr;

            while (length_ > index) {
                switch (state) {
                case PARSE_ELEMENT_TAG: {
                    char input = stream_[index];
                    if (input == ' ' || input == '\r' || input == '\n' || input == '\t') {
                        if (!self->name.empty()) {
                            state = PARSE_ELEMENT_ATTR;
                        }
                        index++;
                    }
                    else if (input == '/') {
                        self->Parse(attr);
                        element->children.push_back(self);
                        return SkipUntil(index, '>');
                    }
                    else if (input == '>') {
                        if (self_closing_tags_.find(self->name) != self_closing_tags_.end()) {
                            element->children.push_back(self);
                            return ++index;
                        }
                        state = PARSE_ELEMENT_VALUE;
                        index++;
                    }
                    else {
                        self->name.append(stream_ + index, 1);
                        index++;
                    }
                }
                                      break;

                case PARSE_ELEMENT_ATTR: {
                    char input = stream_[index];
                    if (input == '>') {
                        if (stream_[index - 1] == '/') {
                            attr.erase(attr.size() - 1);
                            self->Parse(attr);
                            element->children.push_back(self);
                            return ++index;
                        }
                        else if (self_closing_tags_.find(self->name) != self_closing_tags_.end()) {
                            self->Parse(attr);
                            element->children.push_back(self);
                            return ++index;
                        }
                        state = PARSE_ELEMENT_VALUE;
                        index++;
                    }
                    else {
                        attr.append(stream_ + index, 1);
                        index++;
                    }
                }
                                       break;

                case PARSE_ELEMENT_VALUE: {
                    if (self->name == "script" || self->name == "noscript" || self->name == "style") {
                        std::string close = "</" + self->name + ">";

                        size_t pre = index;
                        index = SkipUntil(index, close.c_str());
                        if (index > (pre + close.size()))
                            self->value.append(stream_ + pre, index - pre - close.size());

                        self->Parse(attr);
                        element->children.push_back(self);
                        return index;
                    }

                    char input = stream_[index];
                    if (input == '<') {
                        if (!self->value.empty()) {
                            shared_ptr<HtmlElement> child(new HtmlElement(self));
                            child->name = "plain";
                            child->value.swap(self->value);
                            self->children.push_back(child);
                        }

                        if (stream_[index + 1] == '/') {
                            state = PARSE_ELEMENT_TAG_END;
                        }
                        else {
                            index = ParseElement(index, self);
                        }
                    }
                    else if (input != '\r' && input != '\n' && input != '\t') {
                        self->value.append(stream_ + index, 1);
                        index++;
                    }
                    else {
                        index++;
                    }
                }
                                        break;

                case PARSE_ELEMENT_TAG_END: {
                    index += 2;
                    std::string selfname = self->name + ">";
                    if (strncmp(stream_ + index, selfname.c_str(), selfname.size())) {
                        size_t pre = index;
                        index = SkipUntil(index, ">");
                        std::string value;
                        if (index > (pre + 1))
                            value.append(stream_ + pre, index - pre - 1);
                        else
                            value.append(stream_ + pre, index - pre);

                        shared_ptr<HtmlElement> parent = self->GetParent();
                        while (parent) {
                            if (parent->name == value) {
                                std::cerr << "WARN : element not closed <" << self->name << "> " << std::endl;
                                self->Parse(attr);
                                element->children.push_back(self);
                                return pre - 2;
                            }

                            parent = parent->GetParent();
                        }

                        std::cerr << "WARN : unexpected closed element </" << value << "> for <" << self->name
                            << ">" << std::endl;
                        state = PARSE_ELEMENT_VALUE;
                    }
                    else {
                        self->Parse(attr);
                        element->children.push_back(self);
                        return SkipUntil(index, '>');
                    }
                }
                                          break;
                }
            }
        }

        return index;
    }

    size_t SkipUntil(size_t index, const char* data) {
        while (length_ > index) {
            if (strncmp(stream_ + index, data, strlen(data)) == 0) {
                return index + strlen(data);
            }
            else {
                index++;
            }
        }

        return index;
    }

    size_t SkipUntil(size_t index, const char data) {
        while (length_ > index) {
            if (stream_[index] == data) {
                return ++index;
            }
            else {
                index++;
            }
        }

        return index;
    }

private:
    const char* stream_;
    size_t length_;
    std::set<std::string> self_closing_tags_;
    shared_ptr<HtmlElement> root_;
};

static std::wstring toLowerW(const std::wstring& str)
{
    std::wstring lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}

static std::string toLower(const std::string& str)
{
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}


inline std::string EscapeForXPath(const std::string& value)
{
    // XPath doesn't allow unescaped single quotes inside single-quoted strings.
    // So we switch to using concat() in such cases.
    if (value.find('\'') == std::string::npos) {
        return value;  // safe to embed directly
    }

    std::ostringstream oss;
    oss << "concat(";
    bool first = true;
    std::istringstream ss(value);
    std::string part;
    while (std::getline(ss, part, '\'')) {
        if (!first) oss << ", \"'\", ";
        oss << "'" << part << "'";
        first = false;
    }
    oss << ")";
    return oss.str();
}

#endif

