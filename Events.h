
/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
	extern "C" {
#endif


#ifdef DEFINE_EVENT_ALIASES
	void USB_Event_Stub(void) ATTR_CONST;
	void USB_Event_Stub(void){}
	bool USB_Event_Stub_Bool(void) ATTR_CONST;
	bool USB_Event_Stub_Bool(void){return false;}
	
	#define ALIAS_DEFAULT ATTR_WEAK ATTR_ALIAS(USB_Event_Stub)
	#define ALIAS_DEFAULT_BOOL ATTR_WEAK ATTR_ALIAS(USB_Event_Stub_Bool)

#else
	#define ALIAS_DEFAULT
	#define ALIAS_DEFAULT_BOOL
#endif

/**
Event handlers. These functions are called from ISRs or are otherwise time-critical,
so handle them quickly.
*/
				
/** Callback to handle a control request that was not handled by the library. Return true
 *  if the request has been handled. Returning false will send a STALL to the host.
 */
bool EVENT_USB_Device_ControlRequest(struct USB_Request_Header* req) ALIAS_DEFAULT_BOOL;

/** Event when OUT data is received as part of a control transfer. */
void EVENT_USB_Device_ControlOUT(uint8_t* data, uint8_t len) ALIAS_DEFAULT;

/** Event when the USB configuration is changed. The configuration is stored in
    variable USB_Device_ConfigurationNumber */
void EVENT_USB_Device_ConfigurationChanged(uint8_t config) ALIAS_DEFAULT;

/** Event when an alternate setting for an interface is selected. Return true
    to accept the alternate setting, or FALSE to send a STALL reply */
bool EVENT_USB_Device_SetInterface(uint8_t interface, uint8_t altsetting) ALIAS_DEFAULT_BOOL;

/** Event when the USB bus suspends */
void EVENT_USB_Device_Suspend(void) ALIAS_DEFAULT;

/** Event when the USB bus returns from suspend */
void EVENT_USB_Device_WakeUp(void) ALIAS_DEFAULT;

/** Event when the host resets the device. Called after the library resets the control endpoint */
void EVENT_USB_Device_Reset(void) ALIAS_DEFAULT;

/** Event called on start of frame, if enabled */
void EVENT_USB_Device_StartOfFrame(void) ALIAS_DEFAULT;


/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
	}
#endif

