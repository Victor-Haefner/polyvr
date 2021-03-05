// @author Alexander Rykovanov 2012
/// @email rykovanov.as@gmail.com
/// @brief Opc Ua binary session services.
/// @license GNU LGPL
///
/// Distributed under the GNU LGPL License
/// (See accompanying file LICENSE or copy at
/// http://www.gnu.org/licenses/lgpl.html)
///

#include "VROPCUAvariant.h"
//#include "binary_serialization.h"

#include <opc/ua/protocol/binary/stream.h>
#include <opc/ua/protocol/nodeid.h>
#include <opc/ua/protocol/string_utils.h>
#include <opc/ua/protocol/types.h>
#include <opc/ua/protocol/variant.h>
#include <opc/ua/protocol/variant_visitor.h>
#include <opc/ua/protocol/protocol_auto.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include <iostream>

using namespace OpcUa;


template <typename T>
bool IsValueArray(const std::vector<T> & t)
{
  return t.size() > 1;
}

template <typename T>
void IsValueArray(const std::vector<T> & t, bool & isArray)
{
  isArray = IsValueArray(t);
}

template <typename T>
bool IsNulValue(const std::vector<T> & t)
{
  return t.empty();
}

template <typename T>
void IsNulValue(const std::vector<T> & t, bool & isNul)
{
  isNul = IsNulValue(t);
}

template <typename T>
void RawValueSizeArray(const std::vector<T> & t, std::size_t & size)
{
  size = RawSizeContainer(t);
}


template <typename T>
void RawValueSize(const std::vector<T> & t, std::size_t & size)
{
  if (IsValueArray(t))
    {
      size = RawSizeContainer(t);
    }

  else if (!IsNulValue(t))
    {
      size = RawSize(t.at(0));
    }

  else
    {
      size = 0;
    }
}

template <typename T>
void SerializeValueArray(const std::vector<T> & value, OpcUa::Binary::DataSerializer & stream)
{
  SerializeContainer(stream, value);
}

template <typename T>
void SerializeValue(const std::vector<T> & value, OpcUa::Binary::DataSerializer & stream)
{
  if (IsValueArray(value))
    {
      SerializeContainer(stream, value);
    }

  else if (!IsNulValue(value))
    {
      stream.Serialize(value.at(0));
    }
}

template <typename T>
void DeserializeContainerValue(std::vector<T> & value, OpcUa::Binary::DataDeserializer & stream)
{
  DeserializeContainer(stream, value);
}

template <typename T>
void DeserializeValue(std::vector<T> & value, OpcUa::Binary::DataDeserializer & stream)
{
  T tmp;
  stream.Deserialize(tmp);
  value.push_back(tmp);
}

struct RawSizeVisitor
{
  size_t Result = 0;

  template <typename T>
  void OnContainer(const T & val)
  {
    Result = RawSizeContainer(val);
  }

  template <typename T>
  void OnScalar(const T & val)
  {
    Result = RawSize(val);
  }
};

template<typename T>
bool Compare(const Variant & lhs, const Variant & rhs)
{
  return lhs.As<T>() == rhs.As<T>();
}

template<>
bool Compare<OpcUa::ExtensionObject>(const Variant & lhs, const Variant & rhs)
{
  OpcUa::ExtensionObject eObjl = lhs.As<OpcUa::ExtensionObject>();
  OpcUa::ExtensionObject eObjr = rhs.As<OpcUa::ExtensionObject>();
  return (eObjl.TypeId == eObjr.TypeId) && (eObjl.Encoding == eObjr.Encoding) && (eObjl.Body == eObjr.Body);
}

template<>
bool Compare<std::vector<OpcUa::ExtensionObject>>(const Variant & lhs, const Variant & rhs)
{
  std::vector<OpcUa::ExtensionObject> eObjsl = lhs.As<std::vector<OpcUa::ExtensionObject>>();
  std::vector<OpcUa::ExtensionObject> eObjsr = rhs.As<std::vector<OpcUa::ExtensionObject>>();
  if (eObjsl.size() != eObjsr.size()) return false;
  for (size_t i=0; i<eObjsl.size(); i++) {
     auto eObjl = eObjsl[i];
     auto eObjr = eObjsr[i];
     if (eObjl.TypeId != eObjr.TypeId || eObjl.Encoding != eObjr.Encoding || !(eObjl.Body == eObjr.Body)) return false;
  }
  return true;
}

bool Variant::operator== (const Variant & var) const
{
  if (Value.empty() ^ var.Value.empty())
    {
      return false;
    }

  if (Value.empty() && var.Value.empty())
    {
      return true;
    }

  if (Value.type() != var.Value.type())
    {
      return false;
    }

  using namespace boost;
  const std::type_info & t = Value.type();

  if (t == typeid(bool))
    { return Compare<bool>(*this, var); }

  else if (t == typeid(std::vector<bool>))
    { return Compare<std::vector<bool>>(*this, var); }

  else if (t == typeid(int8_t))
    { return Compare<int8_t>(*this, var); }

  else if (t == typeid(std::vector<int8_t>))
    { return Compare<std::vector<int8_t>>(*this, var); }

  else if (t == typeid(uint8_t))
    { return Compare<uint8_t>(*this, var); }

  else if (t == typeid(std::vector<uint8_t>))
    { return Compare<std::vector<uint8_t>>(*this, var); }

  else if (t == typeid(int16_t))
    { return Compare<int16_t>(*this, var); }

  else if (t == typeid(std::vector<int16_t>))
    { return Compare<std::vector<int16_t>>(*this, var); }

  else if (t == typeid(uint16_t))
    { return Compare<uint16_t>(*this, var); }

  else if (t == typeid(std::vector<uint16_t>))
    { return Compare<std::vector<uint16_t>>(*this, var); }

  else if (t == typeid(int32_t))
    { return Compare<int32_t>(*this, var); }

  else if (t == typeid(std::vector<int32_t>))
    { return Compare<std::vector<int32_t>>(*this, var); }

  else if (t == typeid(uint32_t))
    { return Compare<uint32_t>(*this, var); }

  else if (t == typeid(std::vector<uint32_t>))
    { return Compare<std::vector<uint32_t>>(*this, var); }

  else if (t == typeid(int64_t))
    { return Compare<int64_t>(*this, var); }

  else if (t == typeid(std::vector<int64_t>))
    { return Compare<std::vector<int64_t>>(*this, var); }

  else if (t == typeid(uint64_t))
    { return Compare<uint64_t>(*this, var); }

  else if (t == typeid(std::vector<uint64_t>))
    { return Compare<std::vector<uint64_t>>(*this, var); }

  else if (t == typeid(float))
    { return Compare<float>(*this, var); }

  else if (t == typeid(std::vector<float>))
    { return Compare<std::vector<float>>(*this, var); }

  else if (t == typeid(double))
    { return Compare<double>(*this, var); }

  else if (t == typeid(std::vector<double>))
    { return Compare<std::vector<double>>(*this, var); }

  else if (t == typeid(std::string))
    { return Compare<std::string>(*this, var); }

  else if (t == typeid(std::vector<std::string>))
    { return Compare<std::vector<std::string>>(*this, var); }

  else if (t == typeid(DateTime))
    { return Compare<DateTime>(*this, var); }

  else if (t == typeid(std::vector<DateTime>))
    { return Compare<std::vector<DateTime>>(*this, var); }

  else if (t == typeid(Guid))
    { return Compare<Guid>(*this, var); }

  else if (t == typeid(std::vector<Guid>))
    { return Compare<std::vector<Guid>>(*this, var); }

  else if (t == typeid(ByteString))
    { return Compare<ByteString>(*this, var); }

  else if (t == typeid(std::vector<ByteString>))
    { return Compare<std::vector<ByteString>>(*this, var); }

  else if (t == typeid(NodeId))
    { return Compare<NodeId>(*this, var); }

  else if (t == typeid(std::vector<NodeId>))
    { return Compare<std::vector<NodeId>>(*this, var); }

  else if (t == typeid(StatusCode))
    { return Compare<StatusCode>(*this, var); }

  else if (t == typeid(std::vector<StatusCode>))
    { return Compare<std::vector<StatusCode>>(*this, var); }

  else if (t == typeid(LocalizedText))
    { return Compare<LocalizedText>(*this, var); }

  else if (t == typeid(std::vector<LocalizedText>))
    { return Compare<std::vector<LocalizedText>>(*this, var); }

  else if (t == typeid(QualifiedName))
    { return Compare<QualifiedName>(*this, var); }

  else if (t == typeid(std::vector<QualifiedName>))
    { return Compare<std::vector<QualifiedName>>(*this, var); }

  /*
      else if (t == typeid(DataValue))
        return Compare<DataValue>(*this, Value);
      else if (t == typeid(std::vector<DataValue>))
        return Compare<std::vector<DataValue>>(*this, var);
  */
  else if (t == typeid(Variant))
    { return Compare<Variant>(*this, var); }

  else if (t == typeid(std::vector<Variant>))
    { return Compare<std::vector<Variant>>(*this, var); }

  else if (t == typeid(DiagnosticInfo))
    { return Compare<DiagnosticInfo>(*this, var); }

  else if (t == typeid(std::vector<DiagnosticInfo>))
    { return Compare<std::vector<DiagnosticInfo>>(*this, var); }

  else if (t == typeid(ExtensionObject))
    { return Compare<ExtensionObject>(*this, var); }

  else if (t == typeid(std::vector<ExtensionObject>))
    { return Compare<std::vector<ExtensionObject>>(*this, var); }

  throw std::logic_error(std::string("Unknown variant type '") + t.name() + std::string("' in Variant comparison."));
}

bool Variant::IsScalar() const
{
  return !IsArray();
}

bool Variant::IsNul() const
{
  return Value.empty();
}

bool Variant::IsArray() const
{
  const std::type_info & t = Value.type();
  return
    (t == typeid(std::vector<bool>))       ||
    (t == typeid(std::vector<int8_t>))     ||
    (t == typeid(std::vector<uint8_t>))    ||
    (t == typeid(std::vector<int16_t>))    ||
    (t == typeid(std::vector<uint16_t>))   ||
    (t == typeid(std::vector<int32_t>))    ||
    (t == typeid(std::vector<uint32_t>))   ||
    (t == typeid(std::vector<int64_t>))    ||
    (t == typeid(std::vector<uint64_t>))   ||
    (t == typeid(std::vector<float>))      ||
    (t == typeid(std::vector<double>))     ||
    (t == typeid(std::vector<std::string>)) ||
    (t == typeid(std::vector<DateTime>))   ||
    (t == typeid(std::vector<Guid>))       ||
    (t == typeid(std::vector<ByteString>)) ||
    (t == typeid(std::vector<NodeId>))     ||
    (t == typeid(std::vector<StatusCode>)) ||
    (t == typeid(std::vector<LocalizedText>)) ||
    (t == typeid(std::vector<QualifiedName>)) ||
//    (t == typeid(std::vector<DataValue>))  ||
    (t == typeid(std::vector<Variant>))    ||
    (t == typeid(std::vector<DiagnosticInfo>));
}

VariantType Variant::Type() const
{
  if (Value.empty())
    { return VariantType::NUL; }

  const std::type_info & t = Value.type();

  if (t == typeid(bool) || t == typeid(std::vector<bool>))
    { return VariantType::BOOLEAN; }

  else if (t == typeid(int8_t) || t == typeid(std::vector<int8_t>))
    { return VariantType::SBYTE; }

  else if (t == typeid(uint8_t) || t == typeid(std::vector<uint8_t>))
    { return VariantType::BYTE; }

  else if (t == typeid(int16_t) || t == typeid(std::vector<int16_t>))
    { return VariantType::INT16; }

  else if (t == typeid(uint16_t) || t == typeid(std::vector<uint16_t>))
    { return VariantType::UINT16; }

  else if (t == typeid(int32_t) || t == typeid(std::vector<int32_t>))
    { return VariantType::INT32; }

  else if (t == typeid(uint32_t) || t == typeid(std::vector<uint32_t>))
    { return VariantType::UINT32; }

  else if (t == typeid(int64_t) || t == typeid(std::vector<int64_t>))
    { return VariantType::INT64; }

  else if (t == typeid(uint64_t) || t == typeid(std::vector<uint64_t>))
    { return VariantType::UINT64; }

  else if (t == typeid(float) || t == typeid(std::vector<float>))
    { return VariantType::FLOAT; }

  else if (t == typeid(double) || t == typeid(std::vector<double>))
    { return VariantType::DOUBLE; }

  else if (t == typeid(std::string) || t == typeid(std::vector<std::string>))
    { return VariantType::STRING; }

  else if (t == typeid(DateTime) || t == typeid(std::vector<DateTime>))
    { return VariantType::DATE_TIME; }

  else if (t == typeid(Guid) || t == typeid(std::vector<Guid>))
    { return VariantType::GUId; }

  else if (t == typeid(ByteString) || t == typeid(std::vector<ByteString>))
    { return VariantType::BYTE_STRING; }

  else if (t == typeid(NodeId) || t == typeid(std::vector<NodeId>))
    { return VariantType::NODE_Id; }

  else if (t == typeid(StatusCode) || t == typeid(std::vector<StatusCode>))
    { return VariantType::STATUS_CODE; }

  else if (t == typeid(LocalizedText) || t == typeid(std::vector<LocalizedText>))
    { return VariantType::LOCALIZED_TEXT; }

  else if (t == typeid(QualifiedName) || t == typeid(std::vector<QualifiedName>))
    { return VariantType::QUALIFIED_NAME; }

  /*
      else if (t == typeid(DataValue) || t == typeid(std::vector<DataValue>))
        return VariantType::DATA_VALUE;
  */
  else if (t == typeid(Variant) || t == typeid(std::vector<Variant>))
    { return VariantType::VARIANT; }

  else if (t == typeid(DiagnosticInfo) || t == typeid(std::vector<DiagnosticInfo>))
    { return VariantType::DIAGNOSTIC_INFO; }

  else if (t == typeid(ExtensionObject) || t == typeid(std::vector<ExtensionObject>))
    { return VariantType::EXTENSION_OBJECT; }

  throw std::runtime_error(std::string("Unknown variant type '") + t.name() + "' in Variant Type().");
}


void Variant::Visit(VariantVisitor & visitor) const
{
  using namespace boost;
  const std::type_info & t = Value.type();

  if (t == typeid(bool))
    { visitor.Visit(any_cast<bool>(Value)); }

  else if (t == typeid(std::vector<bool>))
    { visitor.Visit(any_cast<std::vector<bool>>(Value)); }

  else if (t == typeid(int8_t))
    { visitor.Visit(any_cast<int8_t>(Value)); }

  else if (t == typeid(std::vector<int8_t>))
    { visitor.Visit(any_cast<std::vector<int8_t>>(Value)); }

  else if (t == typeid(uint8_t))
    { visitor.Visit(any_cast<uint8_t>(Value)); }

  else if (t == typeid(std::vector<uint8_t>))
    { visitor.Visit(any_cast<std::vector<uint8_t>>(Value)); }

  else if (t == typeid(int16_t))
    { visitor.Visit(any_cast<int16_t>(Value)); }

  else if (t == typeid(std::vector<int16_t>))
    { visitor.Visit(any_cast<std::vector<int16_t>>(Value)); }

  else if (t == typeid(uint16_t))
    { visitor.Visit(any_cast<uint16_t>(Value)); }

  else if (t == typeid(std::vector<uint16_t>))
    { visitor.Visit(any_cast<std::vector<uint16_t>>(Value)); }

  else if (t == typeid(int32_t))
    { visitor.Visit(any_cast<int32_t>(Value)); }

  else if (t == typeid(std::vector<int32_t>))
    { visitor.Visit(any_cast<std::vector<int32_t>>(Value)); }

  else if (t == typeid(uint32_t))
    { visitor.Visit(any_cast<uint32_t>(Value)); }

  else if (t == typeid(std::vector<uint32_t>))
    { visitor.Visit(any_cast<std::vector<uint32_t>>(Value)); }

  else if (t == typeid(int64_t))
    { visitor.Visit(any_cast<int64_t>(Value)); }

  else if (t == typeid(std::vector<int64_t>))
    { visitor.Visit(any_cast<std::vector<int64_t>>(Value)); }

  else if (t == typeid(uint64_t))
    { visitor.Visit(any_cast<uint64_t>(Value)); }

  else if (t == typeid(std::vector<uint64_t>))
    { visitor.Visit(any_cast<std::vector<uint64_t>>(Value)); }

  else if (t == typeid(float))
    { visitor.Visit(any_cast<float>(Value)); }

  else if (t == typeid(std::vector<float>))
    { visitor.Visit(any_cast<std::vector<float>>(Value)); }

  else if (t == typeid(double))
    { visitor.Visit(any_cast<double>(Value)); }

  else if (t == typeid(std::vector<double>))
    { visitor.Visit(any_cast<std::vector<double>>(Value)); }

  else if (t == typeid(std::string))
    { visitor.Visit(any_cast<std::string>(Value)); }

  else if (t == typeid(std::vector<std::string>))
    { visitor.Visit(any_cast<std::vector<std::string>>(Value)); }

  else if (t == typeid(DateTime))
    { visitor.Visit(any_cast<DateTime>(Value)); }

  else if (t == typeid(std::vector<DateTime>))
    { visitor.Visit(any_cast<std::vector<DateTime>>(Value)); }

  else if (t == typeid(Guid))
    { visitor.Visit(any_cast<Guid>(Value)); }

  else if (t == typeid(std::vector<Guid>))
    { visitor.Visit(any_cast<std::vector<Guid>>(Value)); }

  else if (t == typeid(ByteString))
    { visitor.Visit(any_cast<ByteString>(Value)); }

  else if (t == typeid(std::vector<ByteString>))
    { visitor.Visit(any_cast<std::vector<ByteString>>(Value)); }

  else if (t == typeid(NodeId))
    { visitor.Visit(any_cast<NodeId>(Value)); }

  else if (t == typeid(std::vector<NodeId>))
    { visitor.Visit(any_cast<std::vector<NodeId>>(Value)); }

  else if (t == typeid(StatusCode))
    { visitor.Visit(any_cast<StatusCode>(Value)); }

  else if (t == typeid(std::vector<StatusCode>))
    { visitor.Visit(any_cast<std::vector<StatusCode>>(Value)); }

  else if (t == typeid(LocalizedText))
    { visitor.Visit(any_cast<LocalizedText>(Value)); }

  else if (t == typeid(std::vector<LocalizedText>))
    { visitor.Visit(any_cast<std::vector<LocalizedText>>(Value)); }

  else if (t == typeid(QualifiedName))
    { visitor.Visit(any_cast<QualifiedName>(Value)); }

  else if (t == typeid(std::vector<QualifiedName>))
    { visitor.Visit(any_cast<std::vector<QualifiedName>>(Value)); }

  /*
      else if (t == typeid(DataValue))
        visitor.Visit(any_cast<DataValue>(Value));
      else if (t == typeid(std::vector<DataValue>))
        visitor.Visit(any_cast<std::vector<DataValue>>(Value));
        //Variant of variant is not allowed but variant of an array of variant is OK
      else if (t == typeid(Variant))
        visitor.Visit(any_cast<Variant>(Value));
  */
  else if (t == typeid(std::vector<Variant>))
    { visitor.Visit(any_cast<std::vector<Variant>>(Value)); }

  else if (t == typeid(DiagnosticInfo))
    { visitor.Visit(any_cast<DiagnosticInfo>(Value)); }

  else if (t == typeid(std::vector<DiagnosticInfo>))
    { visitor.Visit(any_cast<std::vector<DiagnosticInfo>>(Value)); }

  else if (t == typeid(ExtensionObject))
    {
        OpcUa::ExtensionObject eObj = any_cast<ExtensionObject>(Value);
        visitor.Visit(eObj.Body);
    }

  else if (t == typeid(std::vector<ExtensionObject>))
    {
        auto eObjs = any_cast<std::vector<ExtensionObject>>(Value);
        std::vector<OpcUa::ByteString> bodies;
        for (auto eObj : eObjs) bodies.push_back(eObj.Body);
        visitor.Visit(bodies);
    }

  else
    { throw std::runtime_error(std::string("Unknown variant type '") + t.name() + "' in Variant visit."); }
}

ObjectId VariantTypeToDataType(VariantType vt)
{
  switch (vt)
    {
    case VariantType::BOOLEAN:
      return ObjectId::Boolean;

    case VariantType::SBYTE:
      return ObjectId::SByte;

    case VariantType::BYTE:
      return ObjectId::Byte;

    case VariantType::INT16:
      return ObjectId::Int16;

    case VariantType::UINT16:
      return ObjectId::UInt16;

    case VariantType::INT32:
      return ObjectId::Int32;

    case VariantType::UINT32:
      return ObjectId::UInt32;

    case VariantType::INT64:
      return ObjectId::Int64;

    case VariantType::UINT64:
      return ObjectId::UInt64;

    case VariantType::FLOAT:
      return ObjectId::Float;

    case VariantType::DOUBLE:
      return ObjectId::Double;

    case VariantType::STRING:
      return ObjectId::String;

    case VariantType::DATE_TIME:
      return ObjectId::DateTime;

    case VariantType::GUId:
      return ObjectId::Guid;

    case VariantType::BYTE_STRING:
      return ObjectId::ByteString;

    case VariantType::XML_ELEMENT:
      return ObjectId::XmlElement;

    case VariantType::NODE_Id:
      return ObjectId::NodeId;

    case VariantType::EXPANDED_NODE_Id:
      return ObjectId::ExpandedNodeId;

    case VariantType::STATUS_CODE:
      return ObjectId::StatusCode;

    case VariantType::QUALIFIED_NAME:
      return ObjectId::QualifiedName;

    case VariantType::LOCALIZED_TEXT:
      return ObjectId::LocalizedText;

    case VariantType::DIAGNOSTIC_INFO:
      return ObjectId::DiagnosticInfo;

    case VariantType::DATA_VALUE:
      return ObjectId::DataValue;

    case VariantType::NUL:
      return ObjectId::Null;

    case VariantType::EXTENSION_OBJECT:
    case VariantType::VARIANT:
    default:
    {
      throw std::runtime_error("4Unknown variant type.");
    }
    }
}

VariantType DataTypeToVariantType(const NodeId & dataType)
{
  if (dataType.GetNamespaceIndex())
    {
      std::string msg("Cannot convert to variant type: invalid namespace of node ");
      throw std::runtime_error(msg + ToString(dataType));
    }

  switch (static_cast<OpcUa::ObjectId>(dataType.GetIntegerIdentifier()))
    {
    case ObjectId::Boolean:
      return VariantType::BOOLEAN;

    case ObjectId::SByte:
      return VariantType::SBYTE;

    case ObjectId::Byte:
      return VariantType::BYTE;

    case ObjectId::Int16:
      return VariantType::INT16;

    case ObjectId::UInt16:
      return VariantType::UINT16;

    case ObjectId::Int32:
      return VariantType::INT32;

    case ObjectId::UInt32:
      return VariantType::UINT32;

    case ObjectId::Int64:
      return VariantType::INT64;

    case ObjectId::UInt64:
      return VariantType::UINT64;

    case ObjectId::Float:
      return VariantType::FLOAT;

    case ObjectId::Double:
      return VariantType::DOUBLE;

    case ObjectId::String:
      return VariantType::STRING;

    case ObjectId::DateTime:
      return VariantType::DATE_TIME;

    case ObjectId::Guid:
      return VariantType::GUId;

    case ObjectId::ByteString:
      return VariantType::BYTE_STRING;

    case ObjectId::XmlElement:
      return VariantType::XML_ELEMENT;

    case ObjectId::NodeId:
      return VariantType::NODE_Id;

    case ObjectId::ExpandedNodeId:
      return VariantType::EXPANDED_NODE_Id;

    case ObjectId::StatusCode:
      return VariantType::STATUS_CODE;

    case ObjectId::QualifiedName:
      return VariantType::QUALIFIED_NAME;

    case ObjectId::LocalizedText:
      return VariantType::LOCALIZED_TEXT;

    case ObjectId::DiagnosticInfo:
      return VariantType::DIAGNOSTIC_INFO;

    case ObjectId::DataValue:
      return VariantType::DATA_VALUE;

    case ObjectId::Null:
      return VariantType::NUL;

    default:
      return VariantType::NODE_Id;
      /*
            {
              std::string msg("Unknown type id ");
              throw std::runtime_error(msg  + ToString(dataType));
            }
      */
    }
}

std::string Variant::ToString() const
{
  if (IsScalar())
    {
      std::stringstream str;

      switch (Type())
        {
        case VariantType::DATE_TIME:
          str << OpcUa::ToString(boost::any_cast<DateTime> (Value)); //As<DateTime>());
          break;

        case VariantType::STRING:
          str << boost::any_cast<std::string> (Value);
          break;

        case VariantType::BOOLEAN:
          str << ((boost::any_cast<bool> (Value)) ? "true" : "false");
          break;

        case VariantType::BYTE:
          str << boost::any_cast<unsigned char> (Value);
          break;

        case VariantType::SBYTE:
          str << boost::any_cast<char> (Value);
          break;

        case VariantType::DOUBLE:
          str << boost::any_cast<double> (Value);
          break;

        case VariantType::FLOAT:
          str << boost::any_cast<float> (Value);
          break;

        case VariantType::INT16:
          str << boost::any_cast<int16_t> (Value);
          break;

        case VariantType::INT32:
          str << boost::any_cast<int32_t> (Value);
          break;

        case VariantType::INT64:
          str << boost::any_cast<int64_t> (Value);
          break;

        case VariantType::UINT16:
          str << boost::any_cast<uint16_t> (Value);
          break;

        case VariantType::UINT32:
          str << boost::any_cast<uint32_t> (Value);
          break;

        case VariantType::UINT64:
          str << boost::any_cast<uint64_t> (Value);
          break;

        default:
          str << "conversion to string is not supported";
          break;
        }

      return str.str();
    }

  else
    {
      return "conversion to string is not supported"; //TODO - add implementation for conversion
    }
}
