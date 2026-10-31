#ifndef DIATWEAKS_WINRT_IDS_H
#define DIATWEAKS_WINRT_IDS_H

/* Interface ids and vtable slots from Dia 1.4.0.424's Microsoft.UI.Xaml.winmd.
   A WinRT slot is 6 + the method index on that interface (IInspectable is 0..5).
   These interfaces are stable: WinUI adds methods on a new interface, not this one. */

#define VT_QUERY 0
#define VT_RELEASE 2
#define VT_GET_CLASS_NAME 4

/* IFrameworkElementAutomationPeer.get_Owner, and IAutomationPeerProtected.PeerFromProvider. */
#define VT_GET_OWNER 6
#define VT_PEER_FROM_PROVIDER 6

/* IVisualTreeHelperStatics */
#define VT_GET_CHILD 10
#define VT_GET_CHILD_COUNT 11

/* IFrameworkElement */
#define VT_GET_ACTUAL_WIDTH 13
#define VT_GET_ACTUAL_HEIGHT 14
#define VT_GET_MARGIN 31
#define VT_PUT_MARGIN 32
#define VT_GET_NAME 33
#define VT_GET_PARENT 54

/* IUIElement. Visible = 0, Collapsed = 1. */
#define VT_GET_VISIBILITY 23
#define VT_PUT_VISIBILITY 24
#define VT_GET_OFFSET 107

/* IBorder */
#define VT_BORDER_GET_RADIUS 14
#define VT_BORDER_PUT_RADIUS 15
#define VT_BORDER_GET_PADDING 16
#define VT_BORDER_PUT_PADDING 17

/* IControl */
#define VT_CONTROL_GET_PADDING 34
#define VT_CONTROL_PUT_PADDING 35
#define VT_CONTROL_GET_RADIUS 52
#define VT_CONTROL_PUT_RADIUS 53

#define DIA_IID_UI \
    { 0xC3C01020, 0x320C, 0x5CF6, { 0x9D, 0x24, 0xD3, 0x96, 0xBB, 0xFA, 0x4D, 0x8B } }
#define DIA_IID_FRAMEWORK \
    { 0xFE08F13D, 0xDC6A, 0x5495, { 0xAD, 0x44, 0xC2, 0xD8, 0xD2, 0x18, 0x63, 0xB0 } }
#define DIA_IID_DEPENDENCY \
    { 0xE7BEAEE7, 0x160E, 0x50F7, { 0x87, 0x89, 0xD6, 0x34, 0x63, 0xF9, 0x79, 0xFA } }
#define DIA_IID_BORDER \
    { 0x1CA13B47, 0xFF5C, 0x5ABC, { 0xA4, 0x11, 0xA1, 0x77, 0xDF, 0x94, 0x82, 0xA9 } }
#define DIA_IID_CONTROL \
    { 0x857D6E8A, 0xD45A, 0x5C69, { 0xA9, 0x9C, 0xBF, 0x6A, 0x5C, 0x54, 0xFB, 0x38 } }
#define DIA_IID_TREE \
    { 0x5AECE43C, 0x7651, 0x5BB5, { 0x85, 0x5C, 0x21, 0x98, 0x49, 0x6E, 0x45, 0x5E } }
#define DIA_IID_PEER \
    { 0xE51D3E4E, 0x34F0, 0x568C, { 0x99, 0x9F, 0x62, 0x77, 0xE2, 0xAF, 0xE6, 0xD7 } }
#define DIA_IID_PEER_OWNER \
    { 0x7DAB4F24, 0x605C, 0x51CB, { 0x87, 0xDB, 0x3E, 0xED, 0x1B, 0x9F, 0xB3, 0x7B } }
#define DIA_IID_PEER_PROTECTED \
    { 0x9663B2C3, 0x8C1F, 0x56D4, { 0xAB, 0xD9, 0x26, 0x80, 0x82, 0xA9, 0xE8, 0xBC } }
/* IRawElementProviderSimple */
#define DIA_IID_RAW_PROVIDER \
    { 0xD6DD68D1, 0x86FD, 0x4332, { 0x86, 0x66, 0x9A, 0xBE, 0xDE, 0xA2, 0xD2, 0x4C } }

#endif
