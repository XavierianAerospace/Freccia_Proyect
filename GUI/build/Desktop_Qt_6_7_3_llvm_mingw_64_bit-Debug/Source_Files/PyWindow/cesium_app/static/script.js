console.log("Starting Cesium offline viewer...");

Cesium.Ion.defaultAccessToken = "";
window.CESIUM_BASE_URL = "/static/Cesium/";

const TILESERVER_BASE_URL =
    window.FRECCIA_TILESERVER_BASE_URL || "http://127.0.0.1:8080/styles/basic-preview";

let viewer = null;
let rocketEntity = null;
let flightPath = null;
let hasInitialFix = false;

function numericValue(value, fallback = 0) {
    const parsed = Number(value);
    return Number.isFinite(parsed) ? parsed : fallback;
}

function formatFixed(value, digits) {
    return numericValue(value).toFixed(digits);
}

function buildLabel(payload) {
    return [
        "FRECCIA",
        `Lat ${formatFixed(payload.latitude, 5)}`,
        `Lon ${formatFixed(payload.longitude, 5)}`,
        `Alt ${formatFixed(payload.altitude, 1)} m`
    ].join("\n");
}

function buildDescription(payload) {
    return `
        <b>Latitude:</b> ${formatFixed(payload.latitude, 6)}<br/>
        <b>Longitude:</b> ${formatFixed(payload.longitude, 6)}<br/>
        <b>Altitude:</b> ${formatFixed(payload.altitude, 2)} m<br/>
        <b>Roll:</b> ${formatFixed(payload.roll, 2)} deg<br/>
        <b>Pitch:</b> ${formatFixed(payload.pitch, 2)} deg<br/>
        <b>Yaw:</b> ${formatFixed(payload.yaw, 2)} deg<br/>
        <b>Satellites:</b> ${numericValue(payload.satellites, 0)}<br/>
        <b>HDOP:</b> ${formatFixed(payload.hdop, 2)}
    `;
}

function ensureRocketEntity() {
    if (!viewer || rocketEntity) {
        return;
    }

    flightPath = new Cesium.SampledPositionProperty();

    rocketEntity = viewer.entities.add({
        name: "FRECCIA",
        position: flightPath,
        point: {
            pixelSize: 14,
            color: Cesium.Color.CYAN,
            outlineColor: Cesium.Color.WHITE,
            outlineWidth: 2
        },
        path: {
            resolution: 1,
            leadTime: 0,
            trailTime: 900,
            width: 3,
            material: Cesium.Color.CYAN.withAlpha(0.65)
        },
        label: {
            text: "Esperando telemetria...",
            font: "14px sans-serif",
            fillColor: Cesium.Color.WHITE,
            outlineColor: Cesium.Color.BLACK,
            outlineWidth: 3,
            showBackground: true,
            backgroundColor: Cesium.Color.BLACK.withAlpha(0.55),
            verticalOrigin: Cesium.VerticalOrigin.BOTTOM,
            pixelOffset: new Cesium.Cartesian2(0, -18)
        },
        description: "Esperando telemetria..."
    });
}

function resetTracking() {
    if (!viewer) {
        return;
    }

    if (rocketEntity) {
        viewer.entities.remove(rocketEntity);
    }

    rocketEntity = null;
    flightPath = null;
    hasInitialFix = false;
    ensureRocketEntity();

    viewer.camera.setView({
        destination: Cesium.Cartesian3.fromDegrees(-74.0647, 4.6286, 1500000.0)
    });
}

function applyTelemetry(payload) {
    if (!viewer || !payload) {
        return;
    }

    const latitude = numericValue(payload.latitude, NaN);
    const longitude = numericValue(payload.longitude, NaN);
    const altitude = numericValue(payload.altitude, 0);

    if (!Number.isFinite(latitude) || !Number.isFinite(longitude)) {
        return;
    }

    ensureRocketEntity();

    const now = Cesium.JulianDate.now();
    const position = Cesium.Cartesian3.fromDegrees(longitude, latitude, altitude);

    flightPath.addSample(now, position);
    rocketEntity.label.text = buildLabel(payload);
    rocketEntity.description = buildDescription(payload);
    viewer.clock.currentTime = now;

    if (!hasInitialFix) {
        hasInitialFix = true;
        viewer.flyTo(rocketEntity, {
            duration: 0.0,
            offset: new Cesium.HeadingPitchRange(0.0, Cesium.Math.toRadians(-35.0), 1800000.0)
        });
    }
}

window.updateTelemetryFromPython = function updateTelemetryFromPython(payload) {
    applyTelemetry(payload);
};

window.updateRocketFromQt = window.updateTelemetryFromPython;

window.updatePosition = function updatePosition(lat, lon, alt) {
    applyTelemetry({
        latitude: lat,
        longitude: lon,
        altitude: alt
    });
};

window.resetTelemetryFromPython = function resetTelemetryFromPython() {
    resetTracking();
};

window.resetRocketFromQt = window.resetTelemetryFromPython;

try {
    viewer = new Cesium.Viewer("cesiumContainer", {
        imageryProvider: false,
        baseLayerPicker: false,
        geocoder: false,
        homeButton: false,
        infoBox: true,
        sceneModePicker: false,
        selectionIndicator: false,
        navigationHelpButton: false,
        timeline: false,
        animation: false,
        fullscreenButton: false,
        vrButton: false
    });

    const imageryProvider = new Cesium.UrlTemplateImageryProvider({
        url: `${TILESERVER_BASE_URL}/{z}/{x}/{y}.png`,
        minimumLevel: 0,
        maximumLevel: 14,
        credit: "OpenFreeMap + TileServer-GL"
    });

    viewer.imageryLayers.addImageryProvider(imageryProvider);
    viewer.scene.globe.enableLighting = false;
    viewer.cesiumWidget.creditContainer.style.display = "none";

    ensureRocketEntity();

    viewer.camera.setView({
        destination: Cesium.Cartesian3.fromDegrees(-74.0647, 4.6286, 1500000.0)
    });

    console.log("Cesium map ready.");
} catch (error) {
    console.error("Cesium startup failed:", error);
    document.body.innerHTML =
        '<h1 style="color:red;padding:40px;font-family:sans-serif;">Map error: ' +
        error.message +
        "</h1>";
}
