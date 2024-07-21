import QtQuick 2.15
import QtQuick.Controls 2.15
import QtLocation 5.15
import QtPositioning 5.15

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("Arduino Data Display")

    property real yawTersaring: 0
    property real pitchTersaring: 0
    property real rollTersaring: 0
    property real alpha: 0.1 // Faktor penyaringan untuk EMA
    property string searchLocation: "" // Menyimpan lokasi pencarian

    property var destination: QtPositioning.coordinate() // Menyimpan koordinat tujuan
    property var waypoints: [
        QtPositioning.coordinate(mainWindow.latitude, mainWindow.longitude),
        QtPositioning.coordinate(mainWindow.latitude, mainWindow.longitude)
    ]

    // Fungsi untuk menerapkan EMA pada yaw
    function terapkanEMA(namaProperti, nilaiBaru) {
        root[namaProperti] = root.alpha * nilaiBaru + (1 - root.alpha) * root[namaProperti]
    }

    Plugin {
        id: mapPlugin
        name: "osm" // Menggunakan OpenStreetMap
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(mainWindow.latitude, mainWindow.longitude)
        zoomLevel: 14 // Sesuaikan level zoom yang diinginkan

        // Menampilkan gambar placeholder.png di lokasi GPS
        MapQuickItem {
             id: gpsMarker
             coordinate: QtPositioning.coordinate(mainWindow.latitude, mainWindow.longitude)
             sourceItem: Image {
                 source: "qrc:/placeholder.png" // Sesuaikan dengan path gambar placeholder
                 width: 20
                 height: 20
            }
        }

        MapPolyline {
            id: routePolyline
            line.width: 5
            line.color: 'blue'
            path: waypoints
        }

        // Menampilkan penanda lokasi tujuan
        MapQuickItem {
            id: destinationMarker
            coordinate: destination
            sourceItem: Image {
                source: "qrc:/pgs_biru.png" // Path gambar pgs_biru
                width: 15
                height: 20
            }
        }

        // MapItemView untuk menampilkan hasil pencarian lokasi
        MapItemView {
            id: searchModel
            model: PlaceSearchModel {
                plugin: mapPlugin
                searchTerm: searchLocation
            }

            delegate: MapQuickItem {
                anchorPoint.x: image.width / 2
                anchorPoint.y: image.height
                coordinate: QtPositioning.coordinate(model.latitude, model.longitude)
                sourceItem: Image {
                    id: image
                    source: "qrc:/pgs_biru.png"
                    width: 20
                    height: 20
                }
            }
        }

        PinchHandler {
            id: pinch
            target: null
            onActiveChanged: if (active) {
                map.startCentroid = map.toCoordinate(pinch.centroid.position, false)
            }
            onScaleChanged: (delta) => {
                map.zoomLevel += Math.log2(delta)
                map.alignCoordinateToPoint(map.startCentroid, pinch.centroid.position)
            }
            onRotationChanged: (delta) => {
                map.bearing -= delta
                map.alignCoordinateToPoint(map.startCentroid, pinch.centroid.position)
            }
            grabPermissions: PointerHandler.TakeOverForbidden
        }

        // Handler untuk pengaturan zoom menggunakan roda mouse
        WheelHandler {
            id: wheel
            acceptedDevices: Qt.platform.pluginName === "cocoa" || Qt.platform.pluginName === "wayland"
                             ? PointerDevice.Mouse | PointerDevice.TouchPad
                             : PointerDevice.Mouse
            rotationScale: 1/120
            property: "zoomLevel"
        }

        // Handler untuk menggeser peta dengan drag
        DragHandler {
            id: drag
            target: null
            onTranslationChanged: (delta) => map.pan(-delta.x, -delta.y)
        }

        // MouseArea untuk menangani double click pada peta
        MouseArea {
            anchors.fill: parent
            onDoubleClicked: {
                destination = map.toCoordinate(Qt.point(mouse.x, mouse.y));
                waypoints = [
                    QtPositioning.coordinate(mainWindow.latitude, mainWindow.longitude),
                    destination
                ];
            }
        }

        // Shortcut untuk zoom in menggunakan keyboard
        Shortcut {
            enabled: map.zoomLevel < map.maximumZoomLevel
            sequence: StandardKey.ZoomIn
            onActivated: map.zoomLevel = Math.round(map.zoomLevel + 1)
        }

        TextField {
            id: searchInput
            placeholderText: "Cari lokasi..."
            width: 150
            height: 20 // Atur tinggi TextField sesuai keinginan
            anchors {
                top: parent.top
                right: parent.right
                topMargin: 10 // Atur margin dari atas
                rightMargin: 10 // Atur margin dari kanan
            }
            onAccepted: {
                geocodeAddress(text);
            }
        }

        Slider {
            id: zoomSlider
            from: map.minimumZoomLevel
            to: map.maximumZoomLevel
            value: map.zoomLevel
            stepSize: 1
            anchors {
                top: searchInput.bottom
                topMargin: 10
                right: parent.right
                rightMargin: 10
            }
            onValueChanged: {
                map.zoomLevel = value;
            }
        }
    }

    Rectangle {
        width: 350
        height: 160
        radius: 10
        color: Qt.rgba(1, 1, 1, 0.5)
        border.color: "black" // Optional: Tambahkan border untuk memperjelas batas box

        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.bottomMargin: 10
        anchors.rightMargin: 10


    // Gambar kompas di bagian kanan bawah

        Image {
            id: compassImage
            source: "qrc:/compas1.png" // Sesuaikan path gambar kompas Anda
            width: 150
            height: 150
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.bottomMargin: 10
            anchors.rightMargin: 10

            Image {
                id: pesawatImage
                source: "qrc:/z2.png" // Path gambar pesawat
                width: 70
                height: 70
                smooth: true // Menambahkan smooth untuk animasi yang lebih halus
                anchors.centerIn: parent
                opacity: 0.8 // Atur nilai opacity sesuai kebutuhan (0.0 - 1.0)

                transform: [
                    Rotation {
                        id: rotationYaw
                        origin.x: pesawatImage.width / 2
                        origin.y: pesawatImage.height / 2
                        angle: mainWindow.yaw
                        axis { x: 0; y: 0; z: 1 }
                    },
                    Rotation {
                    id: rotationPitch
                    origin.x: pesawatImage.width / 2
                    origin.y: pesawatImage.height / 2
                    angle: mainWindow.pitch
                    axis { x: 1; y: 0; z: 0 }
                    },
                    Rotation {
                        id: rotationRoll
                        origin.x: pesawatImage.width / 2
                        origin.y: pesawatImage.height / 2
                        angle: mainWindow.roll
                        axis { x: 0; y: 1; z: 0 }
                    }
                ]
            // Behavior on transform {
            //     SequentialAnimation {
            //         NumberAnimation {
            //             target: rotationYaw
            //             property: "angle"
            //             duration: 200
            //             easing.type: Easing.InOutQuad
            //         }
            //         NumberAnimation {
            //             target: rotationPitch
            //             property: "angle"
            //             duration: 200
            //             easing.type: Easing.InOutQuad
            //         }
            //         NumberAnimation {
            //             target: rotationRoll
            //             property: "angle"
            //             duration: 200
            //             easing.type: Easing.InOutQuad
            //         }
            //     }
            // }
            }
        }



        Column {
            anchors.right: compassImage.left
            anchors.bottom: compassImage.bottom
            anchors.rightMargin: 10
            anchors.bottomMargin: 15

        // Yaw value text
            Text {
                text: "Yaw: " + mainWindow.yaw.toFixed(2) // Memformat nilai yaw dengan dua desimal
                font.pointSize: 12
            }

        // Pitch value text
            Text {
                text: "Pitch: " + mainWindow.pitch.toFixed(2) // Memformat nilai pitch dengan dua desimal
                font.pointSize: 12
            }

        // Roll value text
            Text {
                text: "Roll: " + mainWindow.roll.toFixed(2) // Memformat nilai roll dengan dua desimal
                font.pointSize: 12
            }

        // Latitude value text
            Text {
                text: "Latitude: " + mainWindow.latitude.toFixed(6) // Memformat nilai latitude dengan enam desimal
                font.pointSize: 12
            }

        // Longitude value text
            Text {
                text: "Longitude: " + mainWindow.longitude.toFixed(6) // Memformat nilai longitude dengan enam desimal
                font.pointSize: 12
            }
        }
    }


    // Fungsi untuk geocode address
     function geocodeAddress(address) {
         var url = "https://nominatim.openstreetmap.org/search?q=" + encodeURIComponent(address) + "&format=json&addressdetails=1";
         console.log("Geocode URL: " + url);
         var request = new XMLHttpRequest();
         request.open("GET", url, true);
         request.onreadystatechange = function() {
             if (request.readyState === XMLHttpRequest.DONE) {
                 if (request.status === 200) {
                     var geocodeResponse = JSON.parse(request.responseText);
                     console.log("Geocode Response: " + request.responseText);
                     if (geocodeResponse.length > 0) {
                         var firstResult = geocodeResponse[0];
                         var latitude = parseFloat(geocodeResponse[0].lat);
                         var longitude = parseFloat(geocodeResponse[0].lon);
                         console.log("Geocoded Latitude: " + latitude + ", Longitude: " + longitude);
                         map.center = QtPositioning.coordinate(latitude, longitude);
                         map.zoomLevel = 14;
                         destination = QtPositioning.coordinate(latitude, longitude);
                         waypoints = [
                             QtPositioning.coordinate(mainWindow.latitude, mainWindow.longitude),
                             destination
                         ];
                     } else {
                         console.log("Location not found");
                     }
                 } else {
                     console.log("Geocoding request failed: " + request.status);
                 }
             }
         }
         request.send();
     }
 }
