// Initialize Cesium Viewer
const viewer = new Cesium.Viewer('cesiumContainer', {
    imageryProvider: new Cesium.UrlTemplateImageryProvider({
        url: 'mbtiles://local/tile/{z}/{x}/{y}',
        maximumLevel: 19,
        credit: '© MapTiler © OpenStreetMap contributors'
    }),
    baseLayerPicker: false,
    geocoder: false,
    homeButton: false,
    infoBox: false,
    sceneModePicker: true,
    selectionIndicator: false,
    navigationHelpButton: false,
    timeline: false,
    animation: false,
    fullscreenButton: false,
    scene3DOnly: true
});

// Remove default logo
viewer.cesiumWidget.creditContainer.style.display = "none";

// Add a marker for the current position
const rocketEntity = viewer.entities.add({
    name: 'Rocket',
    position: Cesium.Cartesian3.fromDegrees(0, 0, 0),
    point: {
        pixelSize: 10,
        color: Cesium.Color.RED,
        outlineColor: Cesium.Color.WHITE,
        outlineWidth: 2
    },
    label: {
        text: 'Rocket',
        font: '14pt monospace',
        style: Cesium.LabelStyle.FILL_AND_OUTLINE,
        outlineWidth: 2,
        verticalOrigin: Cesium.VerticalOrigin.BOTTOM,
        pixelOffset: new Cesium.Cartesian2(0, -9)
    }
});

// Function to update position called from Python
window.updatePosition = function(lat, lon, alt) {
    const position = Cesium.Cartesian3.fromDegrees(lon, lat, alt);
    rocketEntity.position = position;

    // Smoothly fly to the new position if it's the first update or significant change
    // For continuous tracking, we might want to follow the entity
    viewer.camera.flyTo({
        destination: Cesium.Cartesian3.fromDegrees(lon, lat, alt + 1000), // view from 1km above
        duration: 2.0
    });
};

// Global function to set camera follow mode
let followMode = false;
window.setFollowMode = function(enabled) {
    followMode = enabled;
    if (followMode) {
        viewer.trackedEntity = rocketEntity;
    } else {
        viewer.trackedEntity = undefined;
    }
};

console.log("Cesium initialized offline.");
