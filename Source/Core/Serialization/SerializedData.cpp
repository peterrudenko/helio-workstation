/*
    This file is part of Helio Workstation.

    Helio is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Helio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Helio. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Common.h"
#include "SerializedData.h"

class SerializedData::SharedData final : public ReferenceCountedObject
{
public:

    using Ptr = ReferenceCountedObjectPtr<SharedData>;

    explicit SharedData(const Identifier &t) noexcept : type(t) {}

    SharedData(const SharedData &other) :
        ReferenceCountedObject(),
        type(other.type),
        properties(other.properties)
    {
        for (auto *c : other.children)
        {
            auto *child = new SharedData(*c);
            child->parent = this;
            this->children.add(child);
        }
    }

    SharedData &operator= (const SharedData &) = delete;

    ~SharedData()
    {
        jassert(parent == nullptr);
        for (auto i = this->children.size(); --i >= 0;)
        {
            const Ptr c(this->children.getObjectPointerUnchecked(i));
            c->parent = nullptr;
            this->children.remove(i);
        }
    }
    
    SerializedData getChildWithName(const Identifier &typeToMatch) const
    {
        for (auto *s : this->children)
        {
            if (s->type == typeToMatch)
            {
                return SerializedData(*s);
            }
        }

        return {};
    }
    
    bool isAChildOf(const SharedData *possibleParent) const noexcept
    {
        for (auto *p = parent; p != nullptr; p = p->parent)
        {
            if (p == possibleParent)
            {
                return true;
            }
        }

        return false;
    }

    void addChild(SharedData *child, int index)
    {
        if (child != nullptr)
        {
            jassert(child != this && !this->isAChildOf(child));
            jassert(child->parent == nullptr);
            this->children.insert(index, child);
            child->parent = this;
        }
    }

    void appendChild(SharedData *child)
    {
        if (child != nullptr)
        {
            jassert(child != this && !this->isAChildOf(child));
            jassert(child->parent == nullptr);
            this->children.add(child);
            child->parent = this;
        }
    }

    bool isEquivalentTo(const SharedData &other) const noexcept
    {
        if (this->type != other.type
            || this->properties.size() != other.properties.size()
            || this->children.size() != other.children.size()
            || this->properties != other.properties)
        {
            return false;
        }

        for (int i = 0; i < children.size(); ++i)
        {
            if (!children.getObjectPointerUnchecked(i)->isEquivalentTo(*other.children.getObjectPointerUnchecked(i)))
            {
                return false;
            }
        }

        return true;
    }

    XmlElement *createXml() const
    {
        auto *xml = new XmlElement(this->type);
        this->properties.copyToXmlAttributes(*xml);

        for (auto i = this->children.size(); --i >= 0;)
        {
            xml->prependChildElement(this->children.getObjectPointerUnchecked(i)->createXml());
        }

        return xml;
    }

    void writeToStream(OutputStream &output) const
    {
        output.writeString(this->type.toString());
        output.writeCompressedInt(this->properties.size());

        for (int j = 0; j < this->properties.size(); ++j)
        {
            output.writeString(this->properties.getName(j).toString());
            this->properties.getValueAt(j).writeToStream(output);
        }

        output.writeCompressedInt(this->children.size());

        for (auto *c : this->children)
        {
            writeObjectToStream(output, c);
        }
    }

    static void writeObjectToStream(OutputStream &output, const SharedData *data)
    {
        if (data != nullptr)
        {
            data->writeToStream(output);
        }
        else
        {
            output.writeString({});
            output.writeCompressedInt(0);
            output.writeCompressedInt(0);
        }
    }

    const Identifier type;
    NamedValueSet properties;
    ReferenceCountedArray<SharedData> children;
    SharedData *parent = nullptr;

    JUCE_LEAK_DETECTOR(SharedData)
};

SerializedData::SerializedData() noexcept {}

SerializedData::SerializedData(const Identifier &type) :
    data(new SerializedData::SharedData(type))
{
    jassert(type.toString().isNotEmpty());
}

SerializedData::SerializedData(SharedData::Ptr obj) noexcept  : data(std::move(obj)) {}
SerializedData::SerializedData(SharedData &obj) noexcept : data(obj) {}

SerializedData::SerializedData(const SerializedData &other) noexcept :
    data(other.data) {}

SerializedData::SerializedData(SerializedData &&other) noexcept :
    data(std::move(other.data)) {}

SerializedData &SerializedData::operator= (const SerializedData &other)
{
    if (this->data != other.data)
    {
        this->data = other.data;
    }

    return *this;
}

SerializedData::~SerializedData() {}

bool SerializedData::operator== (const SerializedData &other) const noexcept
{
    return this->data == other.data;
}

bool SerializedData::operator!= (const SerializedData &other) const noexcept
{
    return this->data != other.data;
}

bool SerializedData::isEquivalentTo(const SerializedData &other) const
{
    return this->data == other.data
        || (this->data != nullptr && other.data != nullptr
            && this->data->isEquivalentTo(*other.data));
}

bool SerializedData::isValid() const noexcept
{
    return this->data != nullptr;
}

SerializedData SerializedData::createCopy() const
{
    if (this->data != nullptr)
    {
        return SerializedData(*new SharedData(*this->data));
    }

    return {};
}

bool SerializedData::hasType(const Identifier &typeName) const noexcept
{
    return this->data != nullptr && this->data->type == typeName;
}

Identifier SerializedData::getType() const noexcept
{
    return this->data != nullptr ? this->data->type : Identifier();
}

SerializedData SerializedData::getParent() const noexcept
{
    if (this->data != nullptr)
    {
        if (auto *p = this->data->parent)
        {
            return SerializedData(*p);
        }
    }

    return {};
}

static const var &getNullVarRef() noexcept
{
    static var nullVar;
    return nullVar;
}

const var &SerializedData::getProperty(const Identifier &name) const noexcept
{
    return this->data == nullptr ? getNullVarRef() : this->data->properties[name];
}

var SerializedData::getProperty(const Identifier &name, const var &defaultValue) const
{
    return this->data == nullptr ? defaultValue
        : this->data->properties.getWithDefault(name, defaultValue);
}

SerializedData &SerializedData::setProperty(const Identifier &name, const var &newValue)
{
    jassert(name.toString().isNotEmpty()); // Must have a valid property name!
    jassert(this->data != nullptr); // Trying to add a property to a null ValueTree will fail!

    if (this->data != nullptr)
    {
        this->data->properties.set(name, newValue);
    }

    return *this;
}

bool SerializedData::hasProperty(const Identifier &name) const noexcept
{
    return this->data != nullptr && this->data->properties.contains(name);
}

int SerializedData::getNumProperties() const noexcept
{
    return this->data == nullptr ? 0 : this->data->properties.size();
}

Identifier SerializedData::getPropertyName(int index) const noexcept
{
    return this->data == nullptr ? Identifier()
        : this->data->properties.getName(index);
}

int SerializedData::getNumChildren() const noexcept
{
    return this->data == nullptr ? 0 : this->data->children.size();
}

SerializedData SerializedData::getChild(int index) const
{
    if (this->data != nullptr)
    {
        if (auto *c = this->data->children.getObjectPointer(index))
        {
            return SerializedData(*c);
        }
    }

    return {};
}

SerializedData::Iterator::Iterator(const SerializedData &v, bool isEnd)
    : internal(v.data != nullptr ? (isEnd ? v.data->children.end() : v.data->children.begin()) : nullptr) {}

SerializedData::Iterator &SerializedData::Iterator::operator++()
{
    this->internal = static_cast<SharedData**> (this->internal) + 1;
    return *this;
}

bool SerializedData::Iterator::operator== (const Iterator &other) const
{
    return this->internal == other.internal;
}

bool SerializedData::Iterator::operator!= (const Iterator &other) const
{
    return this->internal != other.internal;
}

SerializedData SerializedData::Iterator::operator*() const
{
    return SerializedData(SharedData::Ptr(*static_cast<SharedData**> (internal)));
}

SerializedData::Iterator SerializedData::begin() const noexcept
{
    return Iterator(*this, false);
}

SerializedData::Iterator SerializedData::end() const noexcept
{
    return Iterator(*this, true);
}

SerializedData SerializedData::getChildWithName(const Identifier &type) const
{
    return this->data != nullptr ? this->data->getChildWithName(type) : SerializedData();
}

void SerializedData::addChild(const SerializedData &child, int index)
{
    jassert(this->data != nullptr);

    if (this->data != nullptr)
    {
        this->data->addChild(child.data.get(), index);
    }
}

void SerializedData::appendChild(const SerializedData &child)
{
    jassert(this->data != nullptr);

    if (this->data != nullptr)
    {
        this->data->appendChild(child.data.get());
    }
}

UniquePointer<XmlElement> SerializedData::writeToXml() const
{
    return UniquePointer<XmlElement>(this->data != nullptr ? this->data->createXml() : nullptr);
}

SerializedData SerializedData::readFromXml(const XmlElement &xml)
{
    if (!xml.isTextElement())
    {
        SerializedData v(xml.getTagName());
        v.data->properties.setFromXmlAttributes(xml);

        forEachXmlChildElement(xml, e)
        {
            v.appendChild(readFromXml(*e));
        }

        return v;
    }

    jassertfalse;
    return {};
}

void SerializedData::writeToStream(OutputStream &output) const
{
    SharedData::writeObjectToStream(output, this->data.get());
}

SerializedData SerializedData::readFromStream(InputStream &input)
{
    const auto type = input.readString();

    if (type.isEmpty())
    {
        return {};
    }

    SerializedData v(type);

    const auto numProps = input.readCompressedInt();

    for (int i = 0; i < numProps; ++i)
    {
        const auto name = input.readString();

        if (name.isNotEmpty())
        {
            v.data->properties.set(name, var::readFromStream(input));
        }
        else
        {
            jassertfalse;
        }
    }

    const auto numChildren = input.readCompressedInt();
    v.data->children.ensureStorageAllocated(numChildren);

    for (int i = 0; i < numChildren; ++i)
    {
        const auto child = readFromStream(input);

        if (!child.isValid())
        {
            return v;
        }

        v.data->children.add(child.data);
        child.data->parent = v.data.get();
    }

    return v;
}

SerializedData SerializedData::readFromData(const void *data, size_t numBytes)
{
    MemoryInputStream in(data, numBytes, false);
    return readFromStream(in);
}