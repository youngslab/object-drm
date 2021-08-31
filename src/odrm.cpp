

#include "odrm/odrm.hpp"

namespace od {

auto release(drmModeCrtcPtr crtc) -> void {
  if (crtc)
    drmModeFreeCrtc(crtc);
}

auto release(drmModeEncoderPtr encoder) -> void {
  if (encoder)
    drmModeFreeEncoder(encoder);
}

auto release(drmModeConnectorPtr connector) -> void {
  if (connector)
    drmModeFreeConnector(connector);
}

auto release(drmModeResPtr res) -> void {
  if (res)
    drmModeFreeResources(res);
}

auto release(drmModeModeInfoPtr res) -> void {
  if (res)
    drmModeFreeModeInfo(res);
}

auto release(drmModePlaneResPtr res) -> void {
  if (res)
    drmModeFreePlaneResources(res);
}

auto release(drmModePlanePtr plane) -> void {
  if (plane)
    drmModeFreePlane(plane);
}

auto release(drmModeObjectPropertiesPtr ps) -> void {
  if (ps)
    drmModeFreeObjectProperties(ps);
}

auto release(drmModePropertyPtr p) -> void {
  if (p)
    drmModeFreeProperty(p);
}

auto toTypeName(drmModeCrtcPtr) -> std::string { return "drmModeCtrcPtr"; }

auto toTypeName(drmModeEncoderPtr) -> std::string {
  return "drmModeEncoderPtr";
}

auto toTypeName(drmModeConnectorPtr) -> std::string {
  return "drmModeConnectorPtr";
}

auto toTypeName(drmModeResPtr) -> std::string { return "drmModeResPtr"; }

auto toTypeName(drmModeModeInfoPtr) -> std::string {
  return "drmModeModeInfoPtr";
}

auto toTypeName(drmModePlaneResPtr) -> std::string {
  return "drmModePlaneResPtr";
}

auto toTypeName(drmModePlanePtr) -> std::string { return "drmModePlanePtr"; }

auto toTypeName(drmModeObjectPropertiesPtr) -> std::string {
  return "drmModeObjectPropertiesPtr";
}

auto toTypeName(drmModePropertyPtr) -> std::string {
  return "drmModePropertyPtr";
}

auto toString(PropertyPtr p) -> std::string {
  auto metadata = fmt::format("property: id={}, name={}, flags={:#x} ",
			      p->prop_id, p->name, p->flags);
  auto value = std::string();
  using namespace std::string_literals;
  if (p->count_values) {
    value = ", values=";
    for (int i = 0; i < p->count_values; i++)
      value += fmt::format(", {}", p->values[i]);
  } else if (p->count_blobs) {
    value = ", blobs=?";
  } else if (p->count_enums) {
    value = ", enums=?";
  }
  return metadata + value;
}

auto toString(ModeInfoPtr mode) -> std::string {
  return fmt::format("drmModeModeInfo: type={}, name={}", mode->type,
		     mode->name);
}

auto toString(ConnectorPtr conn) -> std::string {
  return fmt::format("drmModeConnector: id={}, connection={}, encoder_id={}",
		     conn->connector_id, conn->connection, conn->encoder_id);
}

auto toString(EncoderPtr encoder) -> std::string {
  return fmt::format(
      "drmModeEncoder: id={}, crtc_id={}, possible_crtcs={:#b}, type={}",
      encoder->encoder_id, encoder->crtc_id, encoder->possible_crtcs,
      encoder->encoder_type);
}

auto getEncoders(int fd, drmModeConnectorPtr connector)
    -> std::vector<EncoderPtr> {
  auto encoders = std::vector<EncoderPtr>();
  for (int i = 0; i < connector->count_encoders; i++) {
    encoders.emplace_back(fd, connector->encoders[i]);
  }
  return encoders;
}

auto getCrtcs(int fd, drmModeResPtr res) -> std::vector<CrtcPtr> {
  auto crtcs = std::vector<CrtcPtr>();
  for (int i = 0; i < res->count_crtcs; i++) {
    crtcs.emplace_back(fd, res->crtcs[i]);
  }
  return crtcs;
}

auto getPlaneResources(int fd) -> PlaneResPtr { return PlaneResPtr(fd); }

auto getPlanes(int fd, drmModePlaneResPtr res) -> std::vector<PlanePtr> {
  auto planes = std::vector<PlanePtr>();
  for (int i = 0; i < res->count_planes; i++) {
    planes.emplace_back(fd, res->planes[i]);
  }
  return planes;
}

auto getObjectProperties(int fd, uint32_t objectId, uint32_t type)
    -> ObjectPropertiesPtr {
  return ObjectPropertiesPtr(fd, objectId, type);
}

auto getProperties(int fd, drmModeObjectPropertiesPtr objectProperties)
    -> std::vector<PropertyPtr> {
  auto ps = std::vector<PropertyPtr>();
  for (int i = 0; i < objectProperties->count_props; i++) {
    ps.emplace_back(fd, objectProperties->props[i]);
  }
  return ps;
}

auto getResources(int fd) -> ResPtr { return ResPtr(fd); }

auto getConnectors(int fd, drmModeResPtr res) -> std::vector<ConnectorPtr> {
  auto count_connectors = res->count_connectors;
  std::vector<ConnectorPtr> connectors;
  for (int i = 0; i < count_connectors; i++) {
    connectors.emplace_back(fd, res->connectors[i]);
  }
  return connectors;
}

// ModeInfo is not needed to be release.
auto connectorGetModes(drmModeConnectorPtr const &connector)
    -> std::vector<drmModeModeInfoPtr> const {
  auto modes = std::vector<drmModeModeInfoPtr>();
  for (int i = 0; i < connector->count_modes; i++) {
    modes.push_back(connector->modes + i);
  }
  return modes;
}

// pick one of modes
auto chooseBestMode(std::vector<drmModeModeInfoPtr> const &modes)
    -> drmModeModeInfoPtr {
  drmModeModeInfoPtr best;
  auto largestArea = 0;

  for (auto mode : modes) {
    // 1. preferred
    if (mode->type & DRM_MODE_TYPE_PREFERRED) {
      best = mode;
      break;
    }

    // 2. largest aresa
    auto area = mode->hdisplay * mode->vdisplay;
    if (largestArea < area) {
      best = mode;
      largestArea = area;
    }
  }

  return best;
}

auto isCrtcAssigned(uint32_t crtc_id, std::map<uint32_t, bool> const &assigned)
    -> bool {
  return assigned.find(crtc_id) != assigned.end();
}

auto canUse(std::map<uint32_t, bool> const &crtcTable, uint32_t crtc_id)
    -> bool {
  if (crtcTable.find(crtc_id) == crtcTable.end())
    return false;
  return !crtcTable.find(crtc_id)->second;
}

auto chooseCrtc(int fd, EncoderPtr encoder,
		std::map<uint32_t, bool> const &crtcTable)
    -> std::optional<CrtcPtr> {

  if (encoder->crtc_id && canUse(crtcTable, encoder->crtc_id)) {
    return CrtcPtr(fd, encoder->crtc_id);
  }

  for (auto it = crtcTable.begin(); it != crtcTable.end(); it++) {
    auto idx = std::distance(crtcTable.begin(), it);
    if (!(encoder->possible_crtcs & (1 << idx)))
      continue;

    if (it->second) // it used
      continue;

    return CrtcPtr(fd, it->first);
  }

  return std::nullopt;
}

auto chooseCrtc(int fd, ConnectorPtr connector,
		std::map<uint32_t, bool> const &crtcTable)
    -> std::optional<CrtcPtr> {
  if (!(connector->connection & DRM_MODE_CONNECTED))
    return std::nullopt;

  // Check connector has an encoder already.
  uint32_t encoder_id = connector->encoder_id;
  if (encoder_id) {
    auto encoder = EncoderPtr(fd, encoder_id);
    auto crtc = chooseCrtc(fd, encoder, crtcTable);
    if (crtc)
      return crtc;
  }

  // Check all encoders
  auto encoders = getEncoders(fd, connector);
  for (auto &encoder : encoders) {
    auto crtc = chooseCrtc(fd, encoder, crtcTable);
    if (crtc)
      return crtc;
  }

  return std::nullopt;
}

} // namespace od
