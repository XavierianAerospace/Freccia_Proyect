// Coordenadas por defecto - Universidad Javeriana, Bogotá
const DEFAULT_LAT = 4.6286;
const DEFAULT_LNG = -74.0647;
const DEFAULT_ZOOM = 16;

// Elementos DOM
const mapElement = document.getElementById('map');
const loadingElement = document.getElementById('loading');
const loadingText = document.getElementById('loading-text');
const latElement = document.getElementById('lat');
const lngElement = document.getElementById('lng');
const accuracyInfo = document.getElementById('accuracy-info');
const statusElement = document.getElementById('status');

// Variables globales
let map;
let userMarker;
let accuracyCircle;
let isReceivingData = false;

// Icono personalizado con imagen CORREGIDO
const customIcon = L.icon({
    iconUrl: 'assets/Icon.png', // Imagen de drone/marcador
    iconSize: [120, 120], // Tamaño de la imagen
    iconAnchor: [60, 60], // Punto de anclaje (centro exacto) - CORREGIDO
    popupAnchor: [0, -60] // Donde aparece el popup - CORREGIDO
});

// Inicializar el mapa
function initMap(latitude, longitude, zoom = DEFAULT_ZOOM) {
    // Crear el mapa
    map = L.map('map').setView([latitude, longitude], zoom);
    
    // Añadir capa de OpenStreetMap
    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
        attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors',
        maxZoom: 19
    }).addTo(map);
    
    // Añadir marcador con imagen
    userMarker = L.marker([latitude, longitude], {icon: customIcon})
        .addTo(map)
        .bindPopup(`
            <div style="text-align: center;">
                <h3>Posición Actual</h3>
                <p><strong>Latitud:</strong> ${latitude.toFixed(6)}</p>
                <p><strong>Longitud:</strong> ${longitude.toFixed(6)}</p>
            </div>
        `)
        .openPopup();
    
    // Actualizar información en el panel
    updateLocationInfo(latitude, longitude);
    
    // Ocultar pantalla de carga
    setTimeout(() => {
        loadingElement.style.opacity = '0';
        setTimeout(() => {
            loadingElement.style.display = 'none';
        }, 300);
    }, 500);
}

// Actualizar la información de ubicación en el panel
function updateLocationInfo(latitude, longitude, accuracy = null) {
    latElement.textContent = latitude.toFixed(6);
    lngElement.textContent = longitude.toFixed(6);
    
    if (accuracy) {
        accuracyInfo.innerHTML = `Precisión: <strong>±${Math.round(accuracy)} metros</strong>`;
    } else {
        accuracyInfo.innerHTML = '';
    }
    
    if (isReceivingData) {
        statusElement.className = 'status real';
        statusElement.innerHTML = '<span>✅ Recibiendo datos en tiempo real</span>';
    } else {
        statusElement.className = 'status real';
        statusElement.innerHTML = '<span>Datos Correctos</span>';
    }
}

// Función para actualizar la posición desde Python
function updatePosition(latitude, longitude, accuracy = null) {
    isReceivingData = true;
    
    // Si el mapa no está inicializado, inicializarlo
    if (!map) {
        initMap(latitude, longitude);
        return;
    }
    
    // Actualizar la posición del marcador
    userMarker.setLatLng([latitude, longitude]);
    
    // Actualizar el popup
    userMarker.getPopup().setContent(`
        <div style="text-align: center;">
            <h3>Posición Actual</h3>
            <p><strong>Latitud:</strong> ${latitude.toFixed(6)}</p>
            <p><strong>Longitud:</strong> ${longitude.toFixed(6)}</p>
            <p><em>Datos en tiempo real</em></p>
        </div>
    `).openPopup();
    
    // Centrar el mapa en la nueva posición
    map.setView([latitude, longitude], map.getZoom());
    
    // Actualizar información en el panel
    updateLocationInfo(latitude, longitude, accuracy);
    
    // Actualizar o añadir círculo de precisión si se proporciona
    if (accuracy) {
        if (accuracyCircle) {
            accuracyCircle.setLatLng([latitude, longitude]).setRadius(accuracy);
        } else {
            accuracyCircle = L.circle([latitude, longitude], {
                color: '#007bff',
                fillColor: '#007bff',
                fillOpacity: 0.1,
                radius: accuracy
            }).addTo(map);
        }
    }
    
    console.log(`Posición actualizada: ${latitude}, ${longitude}`);
}

// Obtener la ubicación actual del usuario
function getUserLocation() {
    loadingText.textContent = 'Obteniendo tu ubicación...';
    
    if (!navigator.geolocation) {
        loadingText.textContent = 'Geolocalización no soportada. Usando ubicación por defecto.';
        setTimeout(() => {
            initMap(DEFAULT_LAT, DEFAULT_LNG, 13);
        }, 1500);
        return;
    }
    
    // Solicitar ubicación con alta precisión
    navigator.geolocation.getCurrentPosition(
        function(position) {
            const lat = position.coords.latitude;
            const lng = position.coords.longitude;
            const accuracy = position.coords.accuracy;
            
            loadingText.textContent = 'Ubicación detectada!';
            
            // Inicializar mapa con ubicación real
            initMap(lat, lng);
            
            // Añadir círculo de precisión
            accuracyCircle = L.circle([lat, lng], {
                color: '#007bff',
                fillColor: '#007bff',
                fillOpacity: 0.1,
                radius: accuracy
            }).addTo(map);
            
            // Actualizar información con precisión
            updateLocationInfo(lat, lng, accuracy);
        },
        function(error) {
            console.error('Error de geolocalización:', error);
            
            let errorMessage = 'No se pudo obtener tu ubicación';
            switch(error.code) {
                case error.PERMISSION_DENIED:
                    errorMessage = 'Permiso de ubicación denegado';
                    break;
                case error.POSITION_UNAVAILABLE:
                    errorMessage = 'Información de ubicación no disponible';
                    break;
                case error.TIMEOUT:
                    errorMessage = 'Tiempo de espera agotado';
                    break;
            }
            
            loadingText.textContent = `${errorMessage}. Usando ubicación por defecto.`;
            
            // Usar ubicación por defecto
            setTimeout(() => {
                initMap(DEFAULT_LAT, DEFAULT_LNG, 13);
            }, 1500);
        },
        {
            enableHighAccuracy: true,
            timeout: 10000,
            maximumAge: 60000
        }
    );
}

// Inicializar la aplicación cuando se carga la página
document.addEventListener('DOMContentLoaded', function() {
    // Obtener la ubicación del usuario al cargar
    setTimeout(getUserLocation, 500);
    
    // Exponer la función updatePosition globalmente para que Python pueda llamarla
    window.updatePosition = updatePosition;
    
    // También exponer una función para limpiar el estado
    window.clearPosition = function() {
        isReceivingData = false;
        updateLocationInfo(DEFAULT_LAT, DEFAULT_LNG);
        statusElement.className = 'status default';
        statusElement.innerHTML = '<span>📍 Esperando datos...</span>';
    };
});

// Manejar errores de carga de Leaflet
window.addEventListener('error', function(e) {
    if (e.target.tagName === 'SCRIPT' && e.target.src.includes('leaflet')) {
        loadingText.innerHTML = 'Error al cargar el mapa. Verifica tu conexión a internet.';
        console.error('Error cargando Leaflet:', e);
    }
});