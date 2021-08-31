

#pragma once

#include <drm/drm.h>
#include <drm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <unistd.h>
#include <type_traits>
#include <fcntl.h>
#include <string>
#include <map>

#include <fmt/format.h>

#include <raii/raii.hpp>

namespace od {

// type parameter
template <typename T> struct has_id : public std::true_type {};
template <> struct has_id<drmModeResPtr> : public std::false_type {};
template <> struct has_id<drmModePlaneResPtr> : public std::false_type {};
template <typename T> inline constexpr bool has_id_v = has_id<T>::value;

template <typename T> struct has_type : public std::false_type {};
template <>
struct has_type<drmModeObjectPropertiesPtr> : public std::true_type {};
template <typename T> inline constexpr bool has_type_v = has_type<T>::value;

auto release(drmModeCrtcPtr crtc) -> void;
auto release(drmModeEncoderPtr encoder) -> void;
auto release(drmModeConnectorPtr connector) -> void;
auto release(drmModeResPtr res) -> void;
auto release(drmModeModeInfoPtr res) -> void;
auto release(drmModePlaneResPtr res) -> void;
auto release(drmModePlanePtr plane) -> void;
auto release(drmModeObjectPropertiesPtr ps) -> void;
auto release(drmModePropertyPtr p) -> void;

auto toTypeName(drmModeCrtcPtr) -> std::string;
auto toTypeName(drmModeEncoderPtr) -> std::string;
auto toTypeName(drmModeConnectorPtr) -> std::string;
auto toTypeName(drmModeResPtr) -> std::string;
auto toTypeName(drmModeModeInfoPtr) -> std::string;
auto toTypeName(drmModePlaneResPtr) -> std::string;
auto toTypeName(drmModePlanePtr) -> std::string;
auto toTypeName(drmModeObjectPropertiesPtr) -> std::string;
auto toTypeName(drmModePropertyPtr) -> std::string;

template <typename T> static auto create(int fd, uint32_t id) -> T;

template <> auto create<drmModeCrtcPtr>(int fd, uint32_t id) -> drmModeCrtcPtr {
  return drmModeGetCrtc(fd, id);
}
template <>
auto create<drmModeEncoderPtr>(int fd, uint32_t id) -> drmModeEncoderPtr {
  return drmModeGetEncoder(fd, id);
}

template <>
auto create<drmModeConnectorPtr>(int fd, uint32_t id) -> drmModeConnectorPtr {
  return drmModeGetConnector(fd, id);
}

template <>
auto create<drmModePlanePtr>(int fd, uint32_t id) -> drmModePlanePtr {
  return drmModeGetPlane(fd, id);
}

template <>
auto create<drmModePropertyPtr>(int fd, uint32_t id) -> drmModePropertyPtr {
  return drmModeGetProperty(fd, id);
}

template <typename T> static auto create(int fd) -> T;

template <> auto create<drmModeResPtr>(int fd) -> drmModeResPtr {
  return drmModeGetResources(fd);
}

template <> auto create<drmModePlaneResPtr>(int fd) -> drmModePlaneResPtr {
  return drmModeGetPlaneResources(fd);
}

template <typename T>
static auto create(int fd, uint32_t id, uint32_t type) -> T;

template <>
auto create<drmModeObjectPropertiesPtr>(int fd, uint32_t id, uint32_t type)
    -> drmModeObjectPropertiesPtr {
  return drmModeObjectGetProperties(fd, id, type);
}

// The name "Resource" has been accupied. Instead of it, We use Actor.
// T: Should be a pointer type
template <typename T, typename Enable = void>
class DrmPtr : public raii::AutoDeletable<T> {
public:
  DrmPtr(int fd)
      : raii::AutoDeletable<T>(create<T>(fd), [](auto x) { release(x); }),
	_fd(fd) {
    if (this->getVal() == nullptr)
      throw std::runtime_error(
	  fmt::format("Failed to get {} from fd={}, errono={}", toTypeName(T{}),
		      _fd, strerror(errno)));
  }

  T operator->() const { return this->getVal(); }

protected:
  int _fd;
};

template <typename T>
class DrmPtr<T, typename std::enable_if_t<has_id_v<T> && !has_type_v<T>>>
    : public raii::AutoDeletable<T> {
public:
  using raii::AutoDeletable<T>::AutoDeletable;

  DrmPtr(int fd, uint32_t id)
      : raii::AutoDeletable<T>(create<T>(fd, id), [](auto x) { release(x); }),
	_fd(fd), _id(id) {
    if (this->getVal() == nullptr)
      throw std::runtime_error(
	  fmt::format("Failed to get {} from fd={},id={},errono={}",
		      toTypeName(T{}), _fd, _id, strerror(errno)));
  }

  T operator->() const { return this->getVal(); }

  virtual ~DrmPtr() {}

protected:
  int _fd;
  uint32_t _id;
};

template <typename T>
class DrmPtr<T, typename std::enable_if_t<has_id_v<T> && has_type_v<T>>>
    : public raii::AutoDeletable<T> {
public:
  using raii::AutoDeletable<T>::AutoDeletable;

  DrmPtr(int fd, uint32_t id, uint32_t type)
      : raii::AutoDeletable<T>(create<T>(fd, id, type),
			       [](auto x) { release(x); }),
	_fd(fd), _id(id), _type(type) {
    if (this->getVal() == nullptr)
      throw std::runtime_error(
	  fmt::format("Failed to get {} from fd={}, id={}, type={}, errono={}",
		      toTypeName(T{}), _fd, _id, _type, strerror(errno)));
  }

  T operator->() const { return this->getVal(); }

  virtual ~DrmPtr() {}

protected:
  int _fd;
  uint32_t _id;
  uint32_t _type;
};

// --------------------------------------
// Warppersr of DRM Pointer types
// --------------------------------------

using ModeInfoPtr = drmModeModeInfoPtr;

class CrtcPtr : public DrmPtr<drmModeCrtcPtr> {
public:
  using DrmPtr::DrmPtr;
  CrtcPtr(int fd, uint32_t crtc_id) : DrmPtr(fd, crtc_id) {}
};

class EncoderPtr : public DrmPtr<drmModeEncoderPtr> {
public:
  using DrmPtr::DrmPtr;
  EncoderPtr(int fd, uint32_t encoder_id) : DrmPtr(fd, encoder_id) {}
};

class ConnectorPtr : public DrmPtr<drmModeConnectorPtr> {
public:
  using DrmPtr::DrmPtr;
  ConnectorPtr(int fd, uint32_t connector_id) : DrmPtr(fd, connector_id) {}
};

class ResPtr : public DrmPtr<drmModeResPtr> {
public:
  using DrmPtr::DrmPtr;
  ResPtr(int fd) : DrmPtr(fd) {}
};

class PlaneResPtr : public DrmPtr<drmModePlaneResPtr> {
public:
  using DrmPtr::DrmPtr;
  PlaneResPtr(int fd) : DrmPtr(fd) {}
};

class PlanePtr : public DrmPtr<drmModePlanePtr> {
public:
  using DrmPtr::DrmPtr;
  PlanePtr(int fd, uint32_t id) : DrmPtr(fd, id) {}
};

class ObjectPropertiesPtr : public DrmPtr<drmModeObjectPropertiesPtr> {
public:
  using DrmPtr::DrmPtr;
  ObjectPropertiesPtr(int fd, uint32_t id, uint32_t type)
      : DrmPtr(fd, id, type) {}
};

class PropertyPtr : public DrmPtr<drmModePropertyPtr> {
public:
  using DrmPtr::DrmPtr;
  PropertyPtr(int fd, uint32_t id) : DrmPtr(fd, id) {}
};

// ModeInfo is not needed to be release.
auto connectorGetModes(drmModeConnectorPtr const &connector)
    -> std::vector<drmModeModeInfoPtr> const;
// pick one of modes
//
auto chooseBestMode(std::vector<drmModeModeInfoPtr> const &modes)
    -> drmModeModeInfoPtr;

auto getResources(int fd) -> ResPtr;
auto getConnectors(int fd, drmModeResPtr res) -> std::vector<ConnectorPtr>;
auto getEncoders(int fd, drmModeConnectorPtr connector)
    -> std::vector<EncoderPtr>;
auto getCrtcs(int fd, drmModeResPtr res) -> std::vector<CrtcPtr>;
auto choosePossibleCrtc(std::vector<CrtcPtr> const &crtcs,
			std::vector<EncoderPtr> const &encoders) -> CrtcPtr;

auto getPlaneResources(int fd) -> PlaneResPtr;
auto getPlanes(int fd, drmModePlaneResPtr res) -> std::vector<PlanePtr>;
auto getObjectProperties(int fd, uint32_t objectId, uint32_t type)
    -> ObjectPropertiesPtr;
auto getProperties(int fd, drmModeObjectPropertiesPtr objectProperties)
    -> std::vector<PropertyPtr>;

auto toString(PropertyPtr p) -> std::string;
auto toString(ModeInfoPtr mode) -> std::string;
auto toString(ConnectorPtr conn) -> std::string;
auto toString(EncoderPtr encoder) -> std::string;

auto isCrtcAssigned(uint32_t crtc_id, std::map<uint32_t, bool> const &assigned)
    -> bool;
auto canUse(std::map<uint32_t, bool> const &crtcTable, uint32_t crtc_id)
    -> bool;

auto chooseCrtc(int fd, EncoderPtr encoder,
		std::map<uint32_t, bool> const &crtcTable)
    -> std::optional<CrtcPtr>;

auto chooseCrtc(int fd, ConnectorPtr connector,
		std::map<uint32_t, bool> const &crtcTable)
    -> std::optional<CrtcPtr>;
} // namespace od
