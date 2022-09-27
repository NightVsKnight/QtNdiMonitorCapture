#pragma once

// This file is for accessing the HX camera information. Note that these functions do not require the
// NDI|HX drivers to be installed.

// This function allows you to query whether a particular camera on the network is currently registered to
// use NDI|HX v1 (or v2 if a vendor works with NewTek on this). This will query a camera and determine
// whether it is registered. Cameras that are designed to support HX at all times (models coming from the
// vendors) will always return true cameras that require HX to be licensed on them will return the licensing
// state for that camera.
//
// Please note that the URL needs to be prefixed with the camera brand being used. For instance if you wanted
// to determine if a Panasonic camera was registered for NDI|HX then one might call:
//     NDIlib_is_hx_camera_registered("NDI-HX-PANA-PTZ-V1://192.168.1.100");
// 
// Note that this is in the exact format that NDI finder would return under the URL.
PROCESSINGNDILIB_ADVANCED_API
bool NDIlib_is_hx_camera_registered(const char* p_url);

// This allows you to use the NewTek store for registrations. The camera URL specified at the IP address
// should follow the same naming convention as listed above for NDIlib_is_hx_camera_registered(). The serial
// number will be the serial number that is returned from a NewTek store purchase.
// This function requires:
// - The camera to support registration and HX.
// - The NewTek store to be accessible.
// - The serial number to be in the correct format.
PROCESSINGNDILIB_ADVANCED_API
bool NDIlib_newtek_store_register(const char* p_camera_url, const char* p_serial_number);

// This allows a camera to be unregistered. This can be used to move a registration. 
PROCESSINGNDILIB_ADVANCED_API
bool NDIlib_newtek_store_unregister(const char* p_camera_url, const char* p_serial_number);
