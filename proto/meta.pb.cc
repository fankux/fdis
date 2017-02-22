// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: proto/meta.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "proto/meta.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/port.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace fdis {
class RequestMetaDefaultTypeInternal : public ::google::protobuf::internal::ExplicitlyConstructed<RequestMeta> {};
RequestMetaDefaultTypeInternal _RequestMeta_default_instance_;
class ResponseStatusDefaultTypeInternal : public ::google::protobuf::internal::ExplicitlyConstructed<ResponseStatus> {};
ResponseStatusDefaultTypeInternal _ResponseStatus_default_instance_;

namespace {

::google::protobuf::Metadata file_level_metadata[2];

}  // namespace


const ::google::protobuf::uint32* protobuf_Offsets_proto_2fmeta_2eproto() GOOGLE_ATTRIBUTE_COLD;
const ::google::protobuf::uint32* protobuf_Offsets_proto_2fmeta_2eproto() {
  static const ::google::protobuf::uint32 offsets[] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(RequestMeta, _has_bits_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(RequestMeta, _internal_metadata_),
    ~0u,  // no _extensions_
    ~0u,  // no _oneof_case_
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(RequestMeta, service_id_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(RequestMeta, method_id_),
    0,
    1,
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(ResponseStatus, _has_bits_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(ResponseStatus, _internal_metadata_),
    ~0u,  // no _extensions_
    ~0u,  // no _oneof_case_
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(ResponseStatus, status_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(ResponseStatus, message_),
    1,
    0,
  };
  return offsets;
}

static const ::google::protobuf::internal::MigrationSchema schemas[] = {
  { 0, 6, sizeof(RequestMeta)},
  { 8, 14, sizeof(ResponseStatus)},
};

static const ::google::protobuf::internal::DefaultInstanceData file_default_instances[] = {
  {reinterpret_cast<const ::google::protobuf::Message*>(&_RequestMeta_default_instance_), NULL},
  {reinterpret_cast<const ::google::protobuf::Message*>(&_ResponseStatus_default_instance_), NULL},
};

namespace {

void protobuf_AssignDescriptors() {
  protobuf_AddDesc_proto_2fmeta_2eproto();
  ::google::protobuf::MessageFactory* factory = NULL;
  AssignDescriptors(
      "proto/meta.proto", schemas, file_default_instances, protobuf_Offsets_proto_2fmeta_2eproto(), factory,
      file_level_metadata, NULL, NULL);
}

void protobuf_AssignDescriptorsOnce() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::google::protobuf::GoogleOnceInit(&once, &protobuf_AssignDescriptors);
}

void protobuf_RegisterTypes(const ::std::string&) GOOGLE_ATTRIBUTE_COLD;
void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::internal::RegisterAllTypes(file_level_metadata, 2);
}

}  // namespace

void protobuf_ShutdownFile_proto_2fmeta_2eproto() {
  _RequestMeta_default_instance_.Shutdown();
  delete file_level_metadata[0].reflection;
  _ResponseStatus_default_instance_.Shutdown();
  delete file_level_metadata[1].reflection;
}

void protobuf_InitDefaults_proto_2fmeta_2eproto_impl() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::internal::InitProtobufDefaults();
  _RequestMeta_default_instance_.DefaultConstruct();
  _ResponseStatus_default_instance_.DefaultConstruct();
}

void protobuf_InitDefaults_proto_2fmeta_2eproto() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::google::protobuf::GoogleOnceInit(&once, &protobuf_InitDefaults_proto_2fmeta_2eproto_impl);
}
void protobuf_AddDesc_proto_2fmeta_2eproto_impl() {
  protobuf_InitDefaults_proto_2fmeta_2eproto();
  static const char descriptor[] = {
      "\n\020proto/meta.proto\022\004fdis\"4\n\013RequestMeta\022"
      "\022\n\nservice_id\030\001 \002(\005\022\021\n\tmethod_id\030\002 \002(\005\"1"
      "\n\016ResponseStatus\022\016\n\006status\030\001 \002(\005\022\017\n\007mess"
      "age\030\002 \001(\t"
  };
  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
      descriptor, 129);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "proto/meta.proto", &protobuf_RegisterTypes);
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_proto_2fmeta_2eproto);
}

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AddDesc_proto_2fmeta_2eproto_once_);
void protobuf_AddDesc_proto_2fmeta_2eproto() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AddDesc_proto_2fmeta_2eproto_once_,
                 &protobuf_AddDesc_proto_2fmeta_2eproto_impl);
}
// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_proto_2fmeta_2eproto {
  StaticDescriptorInitializer_proto_2fmeta_2eproto() {
    protobuf_AddDesc_proto_2fmeta_2eproto();
  }
} static_descriptor_initializer_proto_2fmeta_2eproto_;

// ===================================================================

#if !defined(_MSC_VER) || _MSC_VER >= 1900
const int RequestMeta::kServiceIdFieldNumber;
const int RequestMeta::kMethodIdFieldNumber;
#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900

RequestMeta::RequestMeta()
  : ::google::protobuf::Message(), _internal_metadata_(NULL) {
  if (GOOGLE_PREDICT_TRUE(this != internal_default_instance())) {
    protobuf_InitDefaults_proto_2fmeta_2eproto();
  }
  SharedCtor();
  // @@protoc_insertion_point(constructor:fdis.RequestMeta)
}
RequestMeta::RequestMeta(const RequestMeta& from)
  : ::google::protobuf::Message(),
      _internal_metadata_(NULL),
      _has_bits_(from._has_bits_),
      _cached_size_(0) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::memcpy(&service_id_, &from.service_id_,
    reinterpret_cast<char*>(&method_id_) -
    reinterpret_cast<char*>(&service_id_) + sizeof(method_id_));
  // @@protoc_insertion_point(copy_constructor:fdis.RequestMeta)
}

void RequestMeta::SharedCtor() {
  _cached_size_ = 0;
  ::memset(&service_id_, 0, reinterpret_cast<char*>(&method_id_) -
    reinterpret_cast<char*>(&service_id_) + sizeof(method_id_));
}

RequestMeta::~RequestMeta() {
  // @@protoc_insertion_point(destructor:fdis.RequestMeta)
  SharedDtor();
}

void RequestMeta::SharedDtor() {
}

void RequestMeta::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* RequestMeta::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return file_level_metadata[0].descriptor;
}

const RequestMeta& RequestMeta::default_instance() {
  protobuf_InitDefaults_proto_2fmeta_2eproto();
  return *internal_default_instance();
}

RequestMeta* RequestMeta::New(::google::protobuf::Arena* arena) const {
  RequestMeta* n = new RequestMeta;
  if (arena != NULL) {
    arena->Own(n);
  }
  return n;
}

void RequestMeta::Clear() {
// @@protoc_insertion_point(message_clear_start:fdis.RequestMeta)
  if (_has_bits_[0 / 32] & 3u) {
    ::memset(&service_id_, 0, reinterpret_cast<char*>(&method_id_) -
      reinterpret_cast<char*>(&service_id_) + sizeof(method_id_));
  }
  _has_bits_.Clear();
  _internal_metadata_.Clear();
}

bool RequestMeta::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!GOOGLE_PREDICT_TRUE(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:fdis.RequestMeta)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoffNoLastTag(127u);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // required int32 service_id = 1;
      case 1: {
        if (tag == 8u) {
          set_has_service_id();
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &service_id_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // required int32 method_id = 2;
      case 2: {
        if (tag == 16u) {
          set_has_method_id();
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &method_id_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:fdis.RequestMeta)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:fdis.RequestMeta)
  return false;
#undef DO_
}

void RequestMeta::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:fdis.RequestMeta)
  // required int32 service_id = 1;
  if (has_service_id()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(1, this->service_id(), output);
  }

  // required int32 method_id = 2;
  if (has_method_id()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(2, this->method_id(), output);
  }

  if (_internal_metadata_.have_unknown_fields()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
  // @@protoc_insertion_point(serialize_end:fdis.RequestMeta)
}

::google::protobuf::uint8* RequestMeta::InternalSerializeWithCachedSizesToArray(
    bool deterministic, ::google::protobuf::uint8* target) const {
  (void)deterministic; // Unused
  // @@protoc_insertion_point(serialize_to_array_start:fdis.RequestMeta)
  // required int32 service_id = 1;
  if (has_service_id()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(1, this->service_id(), target);
  }

  // required int32 method_id = 2;
  if (has_method_id()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(2, this->method_id(), target);
  }

  if (_internal_metadata_.have_unknown_fields()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:fdis.RequestMeta)
  return target;
}

size_t RequestMeta::RequiredFieldsByteSizeFallback() const {
// @@protoc_insertion_point(required_fields_byte_size_fallback_start:fdis.RequestMeta)
  size_t total_size = 0;

  if (has_service_id()) {
    // required int32 service_id = 1;
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::Int32Size(
        this->service_id());
  }

  if (has_method_id()) {
    // required int32 method_id = 2;
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::Int32Size(
        this->method_id());
  }

  return total_size;
}
size_t RequestMeta::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:fdis.RequestMeta)
  size_t total_size = 0;

  if (_internal_metadata_.have_unknown_fields()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  if (((_has_bits_[0] & 0x00000003) ^ 0x00000003) == 0) {  // All required fields are present.
    // required int32 service_id = 1;
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::Int32Size(
        this->service_id());

    // required int32 method_id = 2;
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::Int32Size(
        this->method_id());

  } else {
    total_size += RequiredFieldsByteSizeFallback();
  }
  int cached_size = ::google::protobuf::internal::ToCachedSize(total_size);
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = cached_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void RequestMeta::MergeFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:fdis.RequestMeta)
  GOOGLE_DCHECK_NE(&from, this);
  const RequestMeta* source =
      ::google::protobuf::internal::DynamicCastToGenerated<const RequestMeta>(
          &from);
  if (source == NULL) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:fdis.RequestMeta)
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:fdis.RequestMeta)
    MergeFrom(*source);
  }
}

void RequestMeta::MergeFrom(const RequestMeta& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:fdis.RequestMeta)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  if (from._has_bits_[0 / 32] & 3u) {
    if (from.has_service_id()) {
      set_service_id(from.service_id());
    }
    if (from.has_method_id()) {
      set_method_id(from.method_id());
    }
  }
}

void RequestMeta::CopyFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:fdis.RequestMeta)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void RequestMeta::CopyFrom(const RequestMeta& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:fdis.RequestMeta)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool RequestMeta::IsInitialized() const {
  if ((_has_bits_[0] & 0x00000003) != 0x00000003) return false;
  return true;
}

void RequestMeta::Swap(RequestMeta* other) {
  if (other == this) return;
  InternalSwap(other);
}
void RequestMeta::InternalSwap(RequestMeta* other) {
  std::swap(service_id_, other->service_id_);
  std::swap(method_id_, other->method_id_);
  std::swap(_has_bits_[0], other->_has_bits_[0]);
  _internal_metadata_.Swap(&other->_internal_metadata_);
  std::swap(_cached_size_, other->_cached_size_);
}

::google::protobuf::Metadata RequestMeta::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  return file_level_metadata[0];
}

#if PROTOBUF_INLINE_NOT_IN_HEADERS
// RequestMeta

// required int32 service_id = 1;
bool RequestMeta::has_service_id() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
void RequestMeta::set_has_service_id() {
  _has_bits_[0] |= 0x00000001u;
}
void RequestMeta::clear_has_service_id() {
  _has_bits_[0] &= ~0x00000001u;
}
void RequestMeta::clear_service_id() {
  service_id_ = 0;
  clear_has_service_id();
}
::google::protobuf::int32 RequestMeta::service_id() const {
  // @@protoc_insertion_point(field_get:fdis.RequestMeta.service_id)
  return service_id_;
}
void RequestMeta::set_service_id(::google::protobuf::int32 value) {
  set_has_service_id();
  service_id_ = value;
  // @@protoc_insertion_point(field_set:fdis.RequestMeta.service_id)
}

// required int32 method_id = 2;
bool RequestMeta::has_method_id() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
void RequestMeta::set_has_method_id() {
  _has_bits_[0] |= 0x00000002u;
}
void RequestMeta::clear_has_method_id() {
  _has_bits_[0] &= ~0x00000002u;
}
void RequestMeta::clear_method_id() {
  method_id_ = 0;
  clear_has_method_id();
}
::google::protobuf::int32 RequestMeta::method_id() const {
  // @@protoc_insertion_point(field_get:fdis.RequestMeta.method_id)
  return method_id_;
}
void RequestMeta::set_method_id(::google::protobuf::int32 value) {
  set_has_method_id();
  method_id_ = value;
  // @@protoc_insertion_point(field_set:fdis.RequestMeta.method_id)
}

#endif  // PROTOBUF_INLINE_NOT_IN_HEADERS

// ===================================================================

#if !defined(_MSC_VER) || _MSC_VER >= 1900
const int ResponseStatus::kStatusFieldNumber;
const int ResponseStatus::kMessageFieldNumber;
#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900

ResponseStatus::ResponseStatus()
  : ::google::protobuf::Message(), _internal_metadata_(NULL) {
  if (GOOGLE_PREDICT_TRUE(this != internal_default_instance())) {
    protobuf_InitDefaults_proto_2fmeta_2eproto();
  }
  SharedCtor();
  // @@protoc_insertion_point(constructor:fdis.ResponseStatus)
}
ResponseStatus::ResponseStatus(const ResponseStatus& from)
  : ::google::protobuf::Message(),
      _internal_metadata_(NULL),
      _has_bits_(from._has_bits_),
      _cached_size_(0) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  message_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  if (from.has_message()) {
    message_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.message_);
  }
  status_ = from.status_;
  // @@protoc_insertion_point(copy_constructor:fdis.ResponseStatus)
}

void ResponseStatus::SharedCtor() {
  _cached_size_ = 0;
  message_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  status_ = 0;
}

ResponseStatus::~ResponseStatus() {
  // @@protoc_insertion_point(destructor:fdis.ResponseStatus)
  SharedDtor();
}

void ResponseStatus::SharedDtor() {
  message_.DestroyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}

void ResponseStatus::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* ResponseStatus::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return file_level_metadata[1].descriptor;
}

const ResponseStatus& ResponseStatus::default_instance() {
  protobuf_InitDefaults_proto_2fmeta_2eproto();
  return *internal_default_instance();
}

ResponseStatus* ResponseStatus::New(::google::protobuf::Arena* arena) const {
  ResponseStatus* n = new ResponseStatus;
  if (arena != NULL) {
    arena->Own(n);
  }
  return n;
}

void ResponseStatus::Clear() {
// @@protoc_insertion_point(message_clear_start:fdis.ResponseStatus)
  if (has_message()) {
    GOOGLE_DCHECK(!message_.IsDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited()));
    (*message_.UnsafeRawStringPointer())->clear();
  }
  status_ = 0;
  _has_bits_.Clear();
  _internal_metadata_.Clear();
}

bool ResponseStatus::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!GOOGLE_PREDICT_TRUE(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:fdis.ResponseStatus)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoffNoLastTag(127u);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // required int32 status = 1;
      case 1: {
        if (tag == 8u) {
          set_has_status();
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int32, ::google::protobuf::internal::WireFormatLite::TYPE_INT32>(
                 input, &status_)));
        } else {
          goto handle_unusual;
        }
        break;
      }

      // optional string message = 2;
      case 2: {
        if (tag == 18u) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_message()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->message().data(), this->message().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "fdis.ResponseStatus.message");
        } else {
          goto handle_unusual;
        }
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:fdis.ResponseStatus)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:fdis.ResponseStatus)
  return false;
#undef DO_
}

void ResponseStatus::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:fdis.ResponseStatus)
  // required int32 status = 1;
  if (has_status()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt32(1, this->status(), output);
  }

  // optional string message = 2;
  if (has_message()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->message().data(), this->message().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "fdis.ResponseStatus.message");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      2, this->message(), output);
  }

  if (_internal_metadata_.have_unknown_fields()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
  // @@protoc_insertion_point(serialize_end:fdis.ResponseStatus)
}

::google::protobuf::uint8* ResponseStatus::InternalSerializeWithCachedSizesToArray(
    bool deterministic, ::google::protobuf::uint8* target) const {
  (void)deterministic; // Unused
  // @@protoc_insertion_point(serialize_to_array_start:fdis.ResponseStatus)
  // required int32 status = 1;
  if (has_status()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt32ToArray(1, this->status(), target);
  }

  // optional string message = 2;
  if (has_message()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->message().data(), this->message().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "fdis.ResponseStatus.message");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        2, this->message(), target);
  }

  if (_internal_metadata_.have_unknown_fields()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:fdis.ResponseStatus)
  return target;
}

size_t ResponseStatus::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:fdis.ResponseStatus)
  size_t total_size = 0;

  if (_internal_metadata_.have_unknown_fields()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  // required int32 status = 1;
  if (has_status()) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::Int32Size(
        this->status());
  }
  // optional string message = 2;
  if (has_message()) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::StringSize(
        this->message());
  }

  int cached_size = ::google::protobuf::internal::ToCachedSize(total_size);
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = cached_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void ResponseStatus::MergeFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:fdis.ResponseStatus)
  GOOGLE_DCHECK_NE(&from, this);
  const ResponseStatus* source =
      ::google::protobuf::internal::DynamicCastToGenerated<const ResponseStatus>(
          &from);
  if (source == NULL) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:fdis.ResponseStatus)
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:fdis.ResponseStatus)
    MergeFrom(*source);
  }
}

void ResponseStatus::MergeFrom(const ResponseStatus& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:fdis.ResponseStatus)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  if (from._has_bits_[0 / 32] & 3u) {
    if (from.has_message()) {
      set_has_message();
      message_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.message_);
    }
    if (from.has_status()) {
      set_status(from.status());
    }
  }
}

void ResponseStatus::CopyFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:fdis.ResponseStatus)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void ResponseStatus::CopyFrom(const ResponseStatus& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:fdis.ResponseStatus)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool ResponseStatus::IsInitialized() const {
  if ((_has_bits_[0] & 0x00000002) != 0x00000002) return false;
  return true;
}

void ResponseStatus::Swap(ResponseStatus* other) {
  if (other == this) return;
  InternalSwap(other);
}
void ResponseStatus::InternalSwap(ResponseStatus* other) {
  message_.Swap(&other->message_);
  std::swap(status_, other->status_);
  std::swap(_has_bits_[0], other->_has_bits_[0]);
  _internal_metadata_.Swap(&other->_internal_metadata_);
  std::swap(_cached_size_, other->_cached_size_);
}

::google::protobuf::Metadata ResponseStatus::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  return file_level_metadata[1];
}

#if PROTOBUF_INLINE_NOT_IN_HEADERS
// ResponseStatus

// required int32 status = 1;
bool ResponseStatus::has_status() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
void ResponseStatus::set_has_status() {
  _has_bits_[0] |= 0x00000002u;
}
void ResponseStatus::clear_has_status() {
  _has_bits_[0] &= ~0x00000002u;
}
void ResponseStatus::clear_status() {
  status_ = 0;
  clear_has_status();
}
::google::protobuf::int32 ResponseStatus::status() const {
  // @@protoc_insertion_point(field_get:fdis.ResponseStatus.status)
  return status_;
}
void ResponseStatus::set_status(::google::protobuf::int32 value) {
  set_has_status();
  status_ = value;
  // @@protoc_insertion_point(field_set:fdis.ResponseStatus.status)
}

// optional string message = 2;
bool ResponseStatus::has_message() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
void ResponseStatus::set_has_message() {
  _has_bits_[0] |= 0x00000001u;
}
void ResponseStatus::clear_has_message() {
  _has_bits_[0] &= ~0x00000001u;
}
void ResponseStatus::clear_message() {
  message_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  clear_has_message();
}
const ::std::string& ResponseStatus::message() const {
  // @@protoc_insertion_point(field_get:fdis.ResponseStatus.message)
  return message_.GetNoArena();
}
void ResponseStatus::set_message(const ::std::string& value) {
  set_has_message();
  message_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:fdis.ResponseStatus.message)
}
void ResponseStatus::set_message(const char* value) {
  set_has_message();
  message_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:fdis.ResponseStatus.message)
}
void ResponseStatus::set_message(const char* value, size_t size) {
  set_has_message();
  message_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:fdis.ResponseStatus.message)
}
::std::string* ResponseStatus::mutable_message() {
  set_has_message();
  // @@protoc_insertion_point(field_mutable:fdis.ResponseStatus.message)
  return message_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
::std::string* ResponseStatus::release_message() {
  // @@protoc_insertion_point(field_release:fdis.ResponseStatus.message)
  clear_has_message();
  return message_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
void ResponseStatus::set_allocated_message(::std::string* message) {
  if (message != NULL) {
    set_has_message();
  } else {
    clear_has_message();
  }
  message_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), message);
  // @@protoc_insertion_point(field_set_allocated:fdis.ResponseStatus.message)
}

#endif  // PROTOBUF_INLINE_NOT_IN_HEADERS

// @@protoc_insertion_point(namespace_scope)

}  // namespace fdis

// @@protoc_insertion_point(global_scope)
