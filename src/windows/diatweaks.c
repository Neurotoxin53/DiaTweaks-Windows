/* Loaded into Dia.exe only. Dia's window chrome is WinUI 3, so the add-on
   walks that tree and sets Visibility, Margin, Padding, and CornerRadius.
   The MSIX is not modified. Nothing here runs after Dia exits. */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <oaidl.h>

#include "tweak.h"
#include "winrt_ids.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void *HSTRING;
typedef struct Quad {
    double a, b, c, d;
} Quad;
typedef struct Vec3 {
    float x, y, z;
} Vec3;

static const GUID IID_UI = DIA_IID_UI;
static const GUID IID_FE = DIA_IID_FRAMEWORK;
static const GUID IID_DEP = DIA_IID_DEPENDENCY;
static const GUID IID_BORDER = DIA_IID_BORDER;
static const GUID IID_CONTROL = DIA_IID_CONTROL;
static const GUID IID_TREE = DIA_IID_TREE;
static const GUID IID_PEER = DIA_IID_PEER;
static const GUID IID_PEER_OWNER = DIA_IID_PEER_OWNER;
static const GUID IID_PEER_PROTECTED = DIA_IID_PEER_PROTECTED;
static const GUID IID_RAW = DIA_IID_RAW_PROVIDER;
static const GUID IID_UNK = {0, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
static const GUID IID_INSPECT = {0xAF86E2E0, 0xB12D, 0x4C6A, {0x9C, 0x5A, 0xD7, 0xAA, 0x65, 0x10, 0x1E, 0x90}};
/* IContentIslandStatics in Dia 1.4.0.424's Microsoft.UI.winmd. FindAllForCurrentThread is slot 8. */
static const GUID IID_ISLAND_STATICS = {0x7B9EB7CC, 0x8C43, 0x5E0A, {0xAB, 0x23, 0xAB, 0x48, 0x62, 0x8F, 0xD2, 0x23}};
static const GUID IID_WINDOW = {0x61F0EC79, 0x5D52, 0x56B5, {0x86, 0xFB, 0x40, 0xFA, 0x4A, 0xF2, 0x88, 0xB0}};
static const GUID IID_WINDOW_STATICS = {0x8CC985E3, 0xA41A, 0x5DF4, {0xB5, 0x31, 0xD3, 0xA1, 0x78, 0x8D, 0x86, 0xC5}};
static const GUID IID_XAML_ROOT = {0x60CB215A, 0xAD15, 0x520A, {0x8B, 0x01, 0x44, 0x16, 0x82, 0x4F, 0x04, 0x41}};
static const GUID IID_XAML_ISLAND = {0x845A5C62, 0xB0F3, 0x5DB8, {0xB4, 0xFF, 0x41, 0x42, 0xBB, 0xD8, 0xA0, 0x44}};
static const GUID IID_DWXS = {0x553AF92C, 0x1381, 0x51D6, {0xBE, 0xE0, 0xF3, 0x4B, 0xEB, 0x04, 0x2E, 0xA8}};
static const GUID IID_FRAG = {0xF7063DA8, 0x8359, 0x439C, {0x92, 0x97, 0xBB, 0xC5, 0x29, 0x9A, 0x7D, 0x87}};
#define VT_ISLAND_FIND_ALL 8
#define VT_ISLAND_GET_APPDATA 7
#define VT_ISLAND_GET_PROPS 10
#define VT_ISLAND_GET_ID 13
#define VT_ISLAND_GET_PROVIDER 25
#define VT_WINDOW_GET_CONTENT 8
#define VT_HOST_GET_CONTENT 6

typedef long (*FnRoInit)(int);
typedef long (*FnActivate)(HSTRING, const GUID *, void **);
typedef long (*FnCreateString)(const wchar_t *, unsigned int, HSTRING *);
typedef long (*FnDeleteString)(HSTRING);
typedef const wchar_t *(*FnRawString)(HSTRING, unsigned int *);
typedef long (*FnAccessible)(HWND, unsigned long, const GUID *, void **);

static FnRoInit g_roinit;
static FnActivate g_activate;
static FnCreateString g_create;
static FnDeleteString g_delete;
static FnRawString g_raw;
static FnAccessible g_acc;
static void *g_tree;
static void *g_peer_factory;
static int g_sta;
static HMODULE g_module;
static CRITICAL_SECTION g_log_lock;
static FILE *g_log;
static wchar_t g_log_path[MAX_PATH];
static HWND g_timer_hwnd;
static volatile HWND g_pending;
static HHOOK g_hook;
static int g_host_ok;

#define MAX_CACHE 96
#define MAX_SEEN 2048

typedef struct Entry {
    void *id, *fe, *ui, *border, *control;
    int depth, tree;
    int have_base;
    int over_vis, over_margin, over_pad, over_rad;
    int logged_vis, logged_margin, logged_pad, logged_rad;
    char name[96];
    char parent[96];
    int has_base_margin, has_base_pad, has_base_rad, has_base_vis;
    int base_vis;
    double base_margin[4], base_pad[4], base_rad[4];
} Entry;

static Entry g_cache[MAX_CACHE];
static int g_ncache;
static Entry g_next[MAX_CACHE];
static int g_nnext;
static void *g_seen[MAX_SEEN];
static int g_nseen;
static int g_cache_full;
static char g_settings_text[8192];
static TweakSettings g_settings;
static int g_tick;
static int g_logged_root;
static int g_have_root;
static int g_detail;
static int g_dumped;
static volatile int g_disabled;
static volatile LONG g_log_held;
static volatile DWORD g_fault_code;
static FILETIME g_json_seen;
static int g_json_seen_ok;
static const char *volatile g_step = "none";
static jmp_buf g_abort;
static volatile LONG g_guard;
static int g_peer_unsupported;
static int g_ndump;
static int g_discover_attempts;
static int g_discover_paused;
static DWORD g_discover_gap;
static DWORD g_last_discover;
static DWORD g_discover_start;
static HWND g_known_tops[16];
static int g_known_top_n;
#define MAX_DUMP 100
typedef struct DumpEl {
    char name[96];
    char cls[80];
    char target[24];
    double w, h;
} DumpEl;
static DumpEl g_dump[MAX_DUMP];
static char g_dump_sig[8192];
static int g_named_logs;
static void *g_island_factory;
static int g_logged_api;

static int local_appdata(wchar_t *out, int cap);

static void *slot(void *obj, int index) { return (*(void ***)obj)[index]; }

static void rel(void *obj) {
    if (!obj) return;
    typedef unsigned long (*Fn)(void *);
    ((Fn)slot(obj, VT_RELEASE))(obj);
}

static void *qi(void *obj, const GUID *iid) {
    void *out = NULL;
    typedef long (*Fn)(void *, const GUID *, void **);
    if (!obj || ((Fn)slot(obj, VT_QUERY))(obj, iid, &out) < 0) return NULL;
    return out;
}

static void log_open(void) {
    if (g_log) return;
    wchar_t root[MAX_PATH], dir[MAX_PATH];
    if (!local_appdata(root, MAX_PATH)) return;
    _snwprintf(dir, MAX_PATH, L"%s\\DiaPatches", root);
    CreateDirectoryW(dir, NULL);
    _snwprintf(g_log_path, MAX_PATH, L"%s\\diatweaks.log", dir);
    g_log = _wfopen(g_log_path, L"a");
}

static void log_line(const char *text) {
    EnterCriticalSection(&g_log_lock);
    g_log_held = 1;
    log_open();
    if (g_log) {
        fseek(g_log, 0, SEEK_END);
        if (ftell(g_log) > 1048576) {
            fclose(g_log);
            g_log = _wfopen(g_log_path, L"w");
            if (g_log) fputs("log truncated at 1 MB\n", g_log);
        }
    }
    if (g_log) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(g_log, "%04u-%02u-%02u %02u:%02u:%02u %s\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
                st.wSecond, text);
        fflush(g_log);
    }
    g_log_held = 0;
    LeaveCriticalSection(&g_log_lock);
}

static void log_msg(const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    log_line(buf);
}

static int ensure_api(void) {
    if (g_activate) return 1;
    if (g_logged_api) return 0;
    HMODULE combase = LoadLibraryW(L"combase.dll");
    HMODULE oleacc = LoadLibraryW(L"oleacc.dll");
    if (!combase || !oleacc) {
        g_logged_api = 1;
        log_line("combase or oleacc is missing");
        return 0;
    }
    /* GetProcAddress returns FARPROC. Copy the pointer so the cast is not a call-type conversion. */
    FARPROC procs[] = {
        GetProcAddress(combase, "RoInitialize"),
        GetProcAddress(combase, "RoGetActivationFactory"),
        GetProcAddress(combase, "WindowsCreateString"),
        GetProcAddress(combase, "WindowsDeleteString"),
        GetProcAddress(combase, "WindowsGetStringRawBuffer"),
        GetProcAddress(oleacc, "AccessibleObjectFromWindow"),
    };
    memcpy(&g_roinit, &procs[0], sizeof g_roinit);
    memcpy(&g_activate, &procs[1], sizeof g_activate);
    memcpy(&g_create, &procs[2], sizeof g_create);
    memcpy(&g_delete, &procs[3], sizeof g_delete);
    memcpy(&g_raw, &procs[4], sizeof g_raw);
    memcpy(&g_acc, &procs[5], sizeof g_acc);
    if (!g_roinit || !g_activate || !g_create || !g_delete || !g_raw || !g_acc) {
        g_logged_api = 1;
        log_line("WinRT entry points are missing");
        return 0;
    }
    return 1;
}

static int ensure_sta(void) {
    if (g_sta) return 1;
    if (!ensure_api()) return 0;
    long hr = g_roinit(0);
    if (hr < 0) {
        log_msg("UI thread refused COM (%lx)", (unsigned long)hr);
        return 0;
    }
    g_sta = 1;
    if (!g_tree) {
        HSTRING name = NULL;
        const wchar_t *cls = L"Microsoft.UI.Xaml.Media.VisualTreeHelper";
        if (g_create(cls, (unsigned)wcslen(cls), &name) >= 0) {
            g_activate(name, &IID_TREE, &g_tree);
            g_delete(name);
        }
        if (!g_tree) log_line("VisualTreeHelper is unavailable");
    }
    return g_tree != NULL;
}

static void copy_hstring(HSTRING hs, char *out, int cap) {
    out[0] = 0;
    if (!hs) return;
    unsigned int len = 0;
    const wchar_t *w = g_raw(hs, &len);
    if (!w) return;
    WideCharToMultiByte(CP_UTF8, 0, w, (int)len, out, cap - 1, NULL, NULL);
    out[cap - 1] = 0;
}

static void runtime_class(void *obj, char *out, int cap) {
    HSTRING hs = NULL;
    typedef long (*Fn)(void *, HSTRING *);
    out[0] = 0;
    void *insp = qi(obj, &IID_INSPECT);
    if (!insp) return;
    if (((Fn)slot(insp, VT_GET_CLASS_NAME))(insp, &hs) >= 0) copy_hstring(hs, out, cap);
    if (hs) g_delete(hs);
    rel(insp);
}

static const char *name_target(const char *name) {
    if (!strcmp(name, "SidebarToggleButton")) return "hideSidebarButton";
    if (!strcmp(name, "ChatButton")) return "hideChatButton";
    if (!strcmp(name, "NavigationBarHost")) return "hideToolbar";
    if (!strcmp(name, "ContentGroup") || !strcmp(name, "ContentHost") || !strcmp(name, "VariantHost")) return "edgeToEdge";
    if (!strcmp(name, "AssistantPanelHost")) return "chat";
    return NULL;
}

static void read_name(void *fe, char *out, int cap) {
    HSTRING hs = NULL;
    typedef long (*Fn)(void *, HSTRING *);
    out[0] = 0;
    if (!fe || ((Fn)slot(fe, VT_GET_NAME))(fe, &hs) < 0) return;
    copy_hstring(hs, out, cap);
    if (hs) g_delete(hs);
}

static int read_double(void *obj, int index, double *out) {
    typedef long (*Fn)(void *, double *);
    return obj && ((Fn)slot(obj, index))(obj, out) >= 0;
}

static int read_quad(void *obj, int index, Quad *out) {
    typedef long (*Fn)(void *, Quad *);
    return obj && ((Fn)slot(obj, index))(obj, out) >= 0;
}

static int write_quad(void *obj, int index, const Quad *value) {
    typedef long (*Fn)(void *, const Quad *);
    return obj && ((Fn)slot(obj, index))(obj, value) >= 0;
}

static int read_vis(void *ui, int *vis) {
    typedef long (*Fn)(void *, int *);
    return ui && ((Fn)slot(ui, VT_GET_VISIBILITY))(ui, vis) >= 0;
}

static int write_vis(void *ui, int vis) {
    typedef long (*Fn)(void *, int);
    return ui && ((Fn)slot(ui, VT_PUT_VISIBILITY))(ui, vis) >= 0;
}

static int child_count(void *dep) {
    int n = 0;
    typedef long (*Fn)(void *, void *, int *);
    static int logged;
    if (!g_tree || !dep) return 0;
    if (((Fn)slot(g_tree, VT_GET_CHILD_COUNT))(g_tree, dep, &n) < 0 || n < 0) return 0;
    if (n > 400) {
        if (!logged) {
            logged = 1;
            log_line("a panel has more than 400 children; the rest are skipped");
        }
        return 400;
    }
    return n;
}

static void *child_at(void *dep, int index) {
    void *child = NULL;
    typedef long (*Fn)(void *, void *, int, void **);
    if (!g_tree || ((Fn)slot(g_tree, VT_GET_CHILD))(g_tree, dep, index, &child) < 0) return NULL;
    return child;
}

static void entry_clear(Entry *entry) {
    rel(entry->id);
    rel(entry->fe);
    rel(entry->ui);
    rel(entry->border);
    rel(entry->control);
    memset(entry, 0, sizeof *entry);
}

static void cache_add(void *obj, void *fe, void *border, void *control, const char *name, const char *parent, int depth,
                      int tree) {
    if (g_nnext >= MAX_CACHE) {
        if (!g_cache_full) {
            g_cache_full = 1;
            log_line("too many candidate elements; some are ignored");
        }
        rel(fe);
        rel(border);
        rel(control);
        return;
    }
    Entry *entry = &g_next[g_nnext++];
    memset(entry, 0, sizeof *entry);
    entry->id = qi(obj, &IID_UNK);
    entry->fe = fe;
    entry->ui = qi(obj, &IID_UI);
    entry->border = border;
    entry->control = control;
    entry->depth = depth;
    entry->tree = tree;
    _snprintf(entry->name, sizeof entry->name, "%s", name ? name : "");
    _snprintf(entry->parent, sizeof entry->parent, "%s", parent ? parent : "");
}

static void walk(void *obj, int depth, const char *parent, int tree) {
    if (!obj || depth > 32) return;
    void *id = qi(obj, &IID_UNK);
    if (!id) return;
    for (int i = 0; i < g_nseen; i++) {
        if (g_seen[i] == id) {
            rel(id);
            return;
        }
    }
    if (g_nseen >= MAX_SEEN) {
        rel(id);
        return;
    }
    g_seen[g_nseen++] = id;

    void *fe = qi(obj, &IID_FE);
    char name[96];
    name[0] = 0;
    double width = 0, height = 0;
    if (fe) {
        read_name(fe, name, sizeof name);
        read_double(fe, VT_GET_ACTUAL_WIDTH, &width);
        read_double(fe, VT_GET_ACTUAL_HEIGHT, &height);
    }
    void *border = NULL, *control = NULL;
    int has_radius = 0;
    if (width >= 200 && height >= 200) {
        border = qi(obj, &IID_BORDER);
        control = qi(obj, &IID_CONTROL);
        Quad rad;
        int got = (border && read_quad(border, VT_BORDER_GET_RADIUS, &rad)) ||
                  (control && read_quad(control, VT_CONTROL_GET_RADIUS, &rad));
        has_radius = got && (rad.a > 0.5 || rad.b > 0.5 || rad.c > 0.5 || rad.d > 0.5);
    }
    TweakElement view;
    memset(&view, 0, sizeof view);
    view.name = name;
    view.parentName = parent ? parent : "";
    view.hasRadius = has_radius;
    view.width = width;
    view.height = height;
    if (fe && (name[0] || has_radius) && g_ndump < MAX_DUMP) {
        DumpEl *el = &g_dump[g_ndump++];
        memset(el, 0, sizeof *el);
        _snprintf(el->name, sizeof el->name, "%s", name);
        runtime_class(fe, el->cls, sizeof el->cls);
        const char *tgt = name_target(name);
        if (tgt) _snprintf(el->target, sizeof el->target, "%s", tgt);
        el->w = width;
        el->h = height;
    }
    if (fe && tweak_interesting(&view)) {
        if (!border) border = qi(obj, &IID_BORDER);
        if (!control) control = qi(obj, &IID_CONTROL);
        cache_add(obj, fe, border, control, name, parent, depth, tree);
    } else {
        rel(fe);
        rel(border);
        rel(control);
    }

    void *dep = qi(obj, &IID_DEP);
    int count = child_count(dep);
    for (int i = 0; i < count; i++) {
        void *child = child_at(dep, i);
        if (child) {
            const char *down = name[0] ? name : (depth == 0 ? "." : (parent && parent[0] ? parent : "."));
            walk(child, depth + 1, down, tree);
            rel(child);
        }
    }
    rel(dep);
}

static int class_skipped(const wchar_t *cls) {
    return wcsncmp(cls, L"Chrome_", 7) == 0 || wcscmp(cls, L"Intermediate D3D Window") == 0 ||
           wcscmp(cls, L"D3DProxyWindow") == 0;
}

static void log_class_once(const wchar_t *cls) {
    static wchar_t seen[8][64];
    static int n;
    for (int i = 0; i < n; i++)
        if (wcscmp(seen[i], cls) == 0) return;
    if (n < 8) _snwprintf(seen[n++], 64, L"%s", cls);
    char utf[128];
    WideCharToMultiByte(CP_UTF8, 0, cls, -1, utf, sizeof utf, NULL, NULL);
    log_msg("window class %s", utf);
}

static void *owner_of_peer(void *peer) {
    void *owner = NULL;
    typedef long (*Fn)(void *, void **);
    long hr = ((Fn)slot(peer, VT_GET_OWNER))(peer, &owner);
    if (g_detail) log_msg("get_Owner hr=%08lx", (unsigned long)hr);
    if (hr < 0) return NULL;
    return owner;
}

static void *element_from_provider(void *provider, const char *via) {
    if (!provider) return NULL;
    char cn[160];
    runtime_class(provider, cn, sizeof cn);
    if (g_detail) log_msg("%s provider class=%s", via, cn[0] ? cn : "(not WinRT)");
    void *direct = qi(provider, &IID_FE);
    if (direct) {
        char name[96];
        read_name(direct, name, sizeof name);
        if (g_detail)
            log_msg("%s provider is FrameworkElement name=%s", via, name[0] ? name : "(unnamed)");
        return direct;
    }
    void *peer = qi(provider, &IID_PEER_OWNER);
    if (!peer) {
        void *as_peer = qi(provider, &IID_PEER);
        if (as_peer) {
            peer = qi(as_peer, &IID_PEER_OWNER);
            rel(as_peer);
        }
    }
    if (!peer && !g_peer_unsupported && ensure_api()) {
        if (!g_peer_factory) {
            HSTRING hs = NULL;
            const wchar_t *cls_name = L"Microsoft.UI.Xaml.Automation.Peers.AutomationPeer";
            long hr = g_create(cls_name, (unsigned)wcslen(cls_name), &hs);
            if (hr >= 0) {
                hr = g_activate(hs, &IID_PEER_PROTECTED, &g_peer_factory);
                g_delete(hs);
            }
            if (!g_peer_factory) {
                g_peer_unsupported = 1;
                if (g_detail) log_msg("AutomationPeer factory hr=%08lx", (unsigned long)hr);
            }
        }
        if (g_peer_factory) {
            void *made = NULL;
            typedef long (*Fn)(void *, void *, void **);
            long hr = ((Fn)slot(g_peer_factory, VT_PEER_FROM_PROVIDER))(g_peer_factory, provider, &made);
            if (g_detail) log_msg("%s PeerFromProvider hr=%08lx", via, (unsigned long)hr);
            if (hr >= 0 && made) {
                peer = qi(made, &IID_PEER_OWNER);
                rel(made);
            }
        }
    }
    if (!peer) {
        if (g_detail) log_msg("%s no FrameworkElementAutomationPeer", via);
        return NULL;
    }
    void *owner = owner_of_peer(peer);
    rel(peer);
    return owner;
}

static void step(const char *name) {
    g_step = name;
    log_msg("step %s", name);
}

static void *content_of(void *face, int slot_index, long *hr_out) {
    void *content = NULL;
    typedef long (*Fn)(void *, void **);
    long hr = ((Fn)slot(face, slot_index))(face, &content);
    if (hr_out) *hr_out = hr;
    if (hr < 0) return NULL;
    return content;
}

/* Return a UIElement from a WinUI window, island, XamlRoot, or the element itself. */
static void *as_element(void *obj, const char *how) {
    if (!obj) return NULL;
    char cn[160];
    runtime_class(obj, cn, sizeof cn);
    if (g_detail) log_msg("%s class=%s", how, cn[0] ? cn : "(not WinRT)");
    void *fe = qi(obj, &IID_FE);
    if (fe) return fe;
    const struct {
        const GUID *iid;
        int slot_index;
        const char *label;
    } faces[] = {
        {&IID_WINDOW, VT_WINDOW_GET_CONTENT, "Window.Content"},
        {&IID_XAML_ISLAND, VT_HOST_GET_CONTENT, "XamlIsland.Content"},
        {&IID_DWXS, VT_HOST_GET_CONTENT, "DesktopWindowXamlSource.Content"},
        {&IID_XAML_ROOT, VT_HOST_GET_CONTENT, "XamlRoot.Content"},
    };
    for (int i = 0; i < 4; i++) {
        void *face = qi(obj, faces[i].iid);
        if (!face) continue;
        long hr = 0;
        void *content = content_of(face, faces[i].slot_index, &hr);
        rel(face);
        if (g_detail) log_msg("%s %s hr=%08lx content=%s", how, faces[i].label, (unsigned long)hr, content ? "yes" : "null");
        if (content) return content;
    }
    return NULL;
}

static void *window_current(void) {
    static void *factory;
    static int tried;
    if (!tried) {
        tried = 1;
        HSTRING hs = NULL;
        const wchar_t *cls = L"Microsoft.UI.Xaml.Window";
        step("window-statics");
        long hr = ensure_api() ? g_create(cls, (unsigned)wcslen(cls), &hs) : (long)0x80004005;
        if (hr >= 0) {
            hr = g_activate(hs, &IID_WINDOW_STATICS, &factory);
            g_delete(hs);
        }
        if (g_detail) log_msg("Window statics hr=%08lx", (unsigned long)hr);
    }
    if (!factory) return NULL;
    void *window = NULL;
    typedef long (*Fn)(void *, void **);
    step("window-get-current");
    long hr = ((Fn)slot(factory, VT_HOST_GET_CONTENT))(factory, &window);
    if (g_detail) log_msg("Window.Current hr=%08lx %s", (unsigned long)hr, window ? "yes" : "null");
    if (hr < 0 || !window) return NULL;
    void *content = as_element(window, "Window.Current");
    rel(window);
    return content;
}

static void uia_text(void *simple, int property, char *out, int cap) {
    VARIANT value;
    typedef long (*Fn)(void *, int, VARIANT *);
    out[0] = 0;
    VariantInit(&value);
    if (((Fn)slot(simple, 4))(simple, property, &value) >= 0 && value.vt == VT_BSTR && value.bstrVal)
        WideCharToMultiByte(CP_UTF8, 0, value.bstrVal, -1, out, cap, NULL, NULL);
    VariantClear(&value);
}

static void uia_walk(void *node, int depth, int *budget) {
    void *frag;
    void *child;
    typedef long (*Fn)(void *, int, void **);
    if (!node || depth > 4 || *budget <= 0) return;
    frag = qi(node, &IID_FRAG);
    if (!frag) return;
    child = NULL;
    if (((Fn)slot(frag, 3))(frag, 3, &child) < 0) child = NULL;
    while (child && *budget > 0) {
        void *simple = qi(child, &IID_RAW);
        void *next = NULL;
        void *child_frag;
        (*budget)--;
        if (simple) {
            char name[96], cls[96];
            uia_text(simple, 30005, name, sizeof name);
            uia_text(simple, 30012, cls, sizeof cls);
            rel(simple);
            if (name[0] || cls[0])
                log_msg("uia-node depth=%d name=%s class=%s", depth, name[0] ? name : "(none)", cls[0] ? cls : "?");
        }
        child_frag = qi(child, &IID_FRAG);
        if (child_frag) {
            uia_walk(child, depth + 1, budget);
            if (((Fn)slot(child_frag, 3))(child_frag, 1, &next) < 0) next = NULL;
            rel(child_frag);
        }
        rel(child);
        child = next;
    }
    rel(frag);
}

static void *g_root_ids[8];
static int g_nroot_ids;
static int g_trees;

static int consider(void *el) {
    void *id;
    if (!el) return 0;
    id = qi(el, &IID_UNK);
    if (!id) {
        rel(el);
        return 0;
    }
    for (int i = 0; i < g_nroot_ids; i++) {
        if (g_root_ids[i] == id) {
            rel(id);
            rel(el);
            return 0;
        }
    }
    if (g_nroot_ids < 8) g_root_ids[g_nroot_ids++] = id;
    else rel(id);
    if (g_detail) {
        char cn[160], name[96];
        runtime_class(el, cn, sizeof cn);
        read_name(el, name, sizeof name);
        log_msg("xaml root class=%s name=%s", cn[0] ? cn : "?", name[0] ? name : "(unnamed)");
    }
    walk(el, 0, "", g_trees++);
    rel(el);
    return 1;
}

static int islands_on_thread(void) {
    if (!g_island_factory) {
        HSTRING hs = NULL;
        const wchar_t *cls = L"Microsoft.UI.Content.ContentIsland";
        step("island-factory");
        long hr = ensure_api() ? g_create(cls, (unsigned)wcslen(cls), &hs) : (long)0x80004005;
        if (hr >= 0) {
            hr = g_activate(hs, &IID_ISLAND_STATICS, &g_island_factory);
            g_delete(hs);
        }
        if (g_detail)
            log_msg("island factory hr=%08lx thread=%lu", (unsigned long)hr, GetCurrentThreadId());
        if (hr < 0 || !g_island_factory) return 0;
    }
    unsigned int count = 0;
    void **items = NULL;
    typedef long (*Fn)(void *, unsigned int *, void ***);
    step("island-findall");
    long hr = ((Fn)slot(g_island_factory, VT_ISLAND_FIND_ALL))(g_island_factory, &count, &items);
    if (g_detail)
        log_msg("island FindAllForCurrentThread hr=%08lx count=%u thread=%lu", (unsigned long)hr, count,
                GetCurrentThreadId());
    if (hr < 0 || !items || count == 0 || count > 64) {
        if (hr >= 0 && count > 64) log_msg("island count %u ignored", count);
        if (items && count > 0 && count <= 64) {
            for (unsigned int i = 0; i < count; i++) rel(items[i]);
        }
        if (items) CoTaskMemFree(items);
        return 0;
    }
    for (unsigned int i = 0; i < count; i++) {
        void *island = items[i];
        unsigned long long id = 0;
        void *app = NULL;
        void *props = NULL;
        void *provider = NULL;
        typedef long (*FnGet)(void *, void **);
        typedef long (*FnId)(void *, unsigned long long *);
        long id_hr, app_hr, props_hr, prov_hr;
        if (!island) continue;
        /* AppData and CustomProperties are the supported reads on this island.
           GetAutomationHostProvider is logged after them; it did not yield XAML. */
        step("island-id");
        id_hr = ((FnId)slot(island, VT_ISLAND_GET_ID))(island, &id);
        if (g_detail) log_msg("island %u id_hr=%08lx id=%llx", i, (unsigned long)id_hr, id);
        step("island-appdata");
        app_hr = ((FnGet)slot(island, VT_ISLAND_GET_APPDATA))(island, &app);
        if (g_detail) log_msg("island %u AppData hr=%08lx", i, (unsigned long)app_hr);
        if (app_hr >= 0 && app) {
            consider(as_element(app, "island.AppData"));
            rel(app);
        }
        step("island-properties");
        props_hr = ((FnGet)slot(island, VT_ISLAND_GET_PROPS))(island, &props);
        if (g_detail) log_msg("island %u CustomProperties hr=%08lx", i, (unsigned long)props_hr);
        if (props_hr >= 0 && props) {
            if (g_detail) {
                char cn[160];
                runtime_class(props, cn, sizeof cn);
                log_msg("island %u CustomProperties class=%s", i, cn[0] ? cn : "(not WinRT)");
            }
            consider(as_element(props, "island.CustomProperties"));
            rel(props);
        }
        step("island-provider");
        prov_hr = ((FnGet)slot(island, VT_ISLAND_GET_PROVIDER))(island, &provider);
        if (g_detail) log_msg("island %u provider_hr=%08lx", i, (unsigned long)prov_hr);
        if (prov_hr >= 0 && provider) {
            char tag[32];
            _snprintf(tag, sizeof tag, "island %u", i);
            void *root = element_from_provider(provider, tag);
            if (root) consider(root);
            else if (g_detail) {
                void *simple = qi(provider, &IID_RAW);
                if (simple) {
                    char name[96], cls[96];
                    uia_text(simple, 30005, name, sizeof name);
                    uia_text(simple, 30012, cls, sizeof cls);
                    rel(simple);
                    log_msg("island %u uia name=%s class=%s", i, name[0] ? name : "(none)", cls[0] ? cls : "?");
                }
                int budget = 40;
                uia_walk(provider, 1, &budget);
            }
            rel(provider);
        }
        rel(island);
    }
    CoTaskMemFree(items);
    return g_trees;
}

static void *root_from_hwnd(HWND hwnd) {
    wchar_t cls[128];
    if (!GetClassNameW(hwnd, cls, 128) || class_skipped(cls)) return NULL;
    log_class_once(cls);
    if (!g_acc) return NULL;
    void *raw = NULL;
    long hr = g_acc(hwnd, 0xFFFFFFFC, &IID_RAW, &raw);
    char utf[128];
    WideCharToMultiByte(CP_UTF8, 0, cls, -1, utf, sizeof utf, NULL, NULL);
    if (g_settings.dry || hr < 0 || !raw)
        log_msg("uia class=%s hwnd=%p acc hr=%08lx", utf, (void *)hwnd, (unsigned long)hr);
    if (hr < 0 || !raw) return NULL;
    void *owner = element_from_provider(raw, "uia");
    rel(raw);
    return owner;
}

typedef struct WinList {
    DWORD pid;
    HWND hwnds[24];
    int count;
} WinList;

static BOOL CALLBACK collect_top(HWND hwnd, LPARAM param) {
    WinList *list = (WinList *)param;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != list->pid || !IsWindowVisible(hwnd) || list->count >= 24) return TRUE;
    RECT rect;
    if (!GetWindowRect(hwnd, &rect)) return TRUE;
    if ((rect.right - rect.left) < 80 || (rect.bottom - rect.top) < 80) return TRUE;
    list->hwnds[list->count++] = hwnd;
    return TRUE;
}

static BOOL CALLBACK collect_child(HWND hwnd, LPARAM param) {
    WinList *list = (WinList *)param;
    if (list->count >= 24) return FALSE;
    wchar_t cls[128];
    if (!GetClassNameW(hwnd, cls, 128)) return TRUE;
    if (!wcsstr(cls, L"Xaml") && !wcsstr(cls, L"WinUI") && !wcsstr(cls, L"DesktopChild")) return TRUE;
    list->hwnds[list->count++] = hwnd;
    return TRUE;
}

static void flush_dump(void);

static void rebuild(void) {
    for (int i = 0; i < g_nseen; i++) rel(g_seen[i]);
    g_nseen = 0;
    g_nnext = 0;
    g_cache_full = 0;
    if (!ensure_sta()) return;

    g_named_logs = 0;
    g_ndump = 0;
    g_dumped = 0;
    g_trees = 0;
    for (int i = 0; i < g_nroot_ids; i++) rel(g_root_ids[i]);
    g_nroot_ids = 0;

    step("window-current");
    consider(window_current());
    step("islands");
    islands_on_thread();
    int trees = g_trees;
    if (!trees && g_detail) {
        WinList tops = {GetCurrentProcessId(), {0}, 0};
        WinList children = {0, {0}, 0};
        step("uia-window");
        EnumWindows(collect_top, (LPARAM)&tops);
        for (int i = 0; i < tops.count; i++) consider(root_from_hwnd(tops.hwnds[i]));
        for (int i = 0; i < tops.count && children.count < 24; i++)
            EnumChildWindows(tops.hwnds[i], collect_child, (LPARAM)&children);
        for (int i = 0; i < children.count; i++) consider(root_from_hwnd(children.hwnds[i]));
        trees = g_trees;
    }
    flush_dump();
    if (!trees && !g_logged_root) {
        g_logged_root = 1;
        log_line("no WinUI root yet; toggles stay off until one appears");
    } else if (trees) {
        if (g_detail) log_msg("found %d WinUI root(s)", trees);
        g_logged_root = 0;
        g_have_root = 1;
    }

    for (int i = 0; i < g_nnext; i++) {
        for (int j = 0; j < g_ncache; j++) {
            if (g_cache[j].id && g_cache[j].id == g_next[i].id && g_cache[j].have_base) {
                Entry *dst = &g_next[i];
                Entry *src = &g_cache[j];
                dst->have_base = 1;
                dst->over_vis = src->over_vis;
                dst->over_margin = src->over_margin;
                dst->over_pad = src->over_pad;
                dst->over_rad = src->over_rad;
                dst->logged_vis = src->logged_vis;
                dst->logged_margin = src->logged_margin;
                dst->logged_pad = src->logged_pad;
                dst->logged_rad = src->logged_rad;
                dst->has_base_margin = src->has_base_margin;
                dst->has_base_pad = src->has_base_pad;
                dst->has_base_rad = src->has_base_rad;
                dst->has_base_vis = src->has_base_vis;
                dst->base_vis = src->base_vis;
                memcpy(dst->base_margin, src->base_margin, sizeof dst->base_margin);
                memcpy(dst->base_pad, src->base_pad, sizeof dst->base_pad);
                memcpy(dst->base_rad, src->base_rad, sizeof dst->base_rad);
                break;
            }
        }
    }
    for (int j = 0; j < g_ncache; j++) entry_clear(&g_cache[j]);
    memcpy(g_cache, g_next, sizeof(Entry) * (size_t)g_nnext);
    g_ncache = g_nnext;
    g_nnext = 0;
    for (int i = 0; i < g_nseen; i++) rel(g_seen[i]);
    g_nseen = 0;
    for (int i = 0; i < g_nroot_ids; i++) rel(g_root_ids[i]);
    g_nroot_ids = 0;
}

static void flush_dump(void) {
    char sig[8192];
    size_t n = 0;
    sig[0] = 0;
    for (int i = 0; i < g_ndump; i++) {
        int wrote = _snprintf(sig + n, sizeof sig - n, "%s|%s;", g_dump[i].name, g_dump[i].target);
        if (wrote < 0 || n + (size_t)wrote >= sizeof sig) break;
        n += (size_t)wrote;
    }
    if (!g_settings.dry || strcmp(sig, g_dump_sig) == 0) return;
    memcpy(g_dump_sig, sig, n + 1);
    g_dumped = 1;
    for (int i = 0; i < g_ndump; i++)
        log_msg("element name=%s class=%s w=%.0f h=%.0f%s%s", g_dump[i].name[0] ? g_dump[i].name : "(unnamed)",
                g_dump[i].cls[0] ? g_dump[i].cls : "?", g_dump[i].w, g_dump[i].h,
                g_dump[i].target[0] ? " target=" : "", g_dump[i].target);
    if (g_ndump == MAX_DUMP) log_line("element dump truncated at 100");
}

typedef struct Live {
    int ok, has_vis, vis, has_margin, has_pad, has_rad, has_place, pad_border, rad_border;
    double margin[4], pad[4], rad[4], offset_x, width, height, parent_width;
} Live;

static void read_live(const Entry *entry, Live *live) {
    memset(live, 0, sizeof *live);
    if (!entry->fe) return;
    live->ok = 1;
    live->has_vis = read_vis(entry->ui, &live->vis);
    Quad q;
    if (read_quad(entry->fe, VT_GET_MARGIN, &q)) {
        live->has_margin = 1;
        live->margin[0] = q.a;
        live->margin[1] = q.b;
        live->margin[2] = q.c;
        live->margin[3] = q.d;
    }
    if (entry->border && read_quad(entry->border, VT_BORDER_GET_PADDING, &q)) {
        live->has_pad = 1;
        live->pad_border = 1;
    } else if (entry->control && read_quad(entry->control, VT_CONTROL_GET_PADDING, &q)) {
        live->has_pad = 1;
    }
    if (live->has_pad) {
        live->pad[0] = q.a;
        live->pad[1] = q.b;
        live->pad[2] = q.c;
        live->pad[3] = q.d;
    }
    if (entry->border && read_quad(entry->border, VT_BORDER_GET_RADIUS, &q)) {
        live->has_rad = 1;
        live->rad_border = 1;
    } else if (entry->control && read_quad(entry->control, VT_CONTROL_GET_RADIUS, &q)) {
        live->has_rad = 1;
    }
    if (live->has_rad) {
        live->rad[0] = q.a;
        live->rad[1] = q.b;
        live->rad[2] = q.c;
        live->rad[3] = q.d;
    }
    read_double(entry->fe, VT_GET_ACTUAL_WIDTH, &live->width);
    read_double(entry->fe, VT_GET_ACTUAL_HEIGHT, &live->height);
    /* ActualOffset is relative to the parent. Sum it up to the WinUI root so a
       corner beside the sidebar is measured from the window, not from an inner grid. */
    void *fe = entry->fe;
    void *ui = entry->ui;
    double x = 0;
    double top_width = live->width;
    typedef long (*FnOff)(void *, Vec3 *);
    typedef long (*FnParent)(void *, void **);
    for (int depth = 0; depth < 10 && fe; depth++) {
        if (ui) {
            Vec3 off;
            if (((FnOff)slot(ui, VT_GET_OFFSET))(ui, &off) >= 0) {
                x += off.x;
                live->has_place = 1;
            }
        }
        void *parent = NULL;
        if (((FnParent)slot(fe, VT_GET_PARENT))(fe, &parent) < 0) parent = NULL;
        void *next_fe = parent ? qi(parent, &IID_FE) : NULL;
        void *next_ui = parent ? qi(parent, &IID_UI) : NULL;
        rel(parent);
        if (next_fe) {
            double width = 0;
            if (read_double(next_fe, VT_GET_ACTUAL_WIDTH, &width)) top_width = width;
        }
        if (fe != entry->fe) rel(fe);
        if (ui && ui != entry->ui) rel(ui);
        fe = next_fe;
        ui = next_ui;
    }
    if (ui && ui != entry->ui) rel(ui);
    if (fe && fe != entry->fe) rel(fe);
    live->offset_x = x;
    live->parent_width = top_width;
}

static void track_baseline(Entry *entry, const Live *live) {
    /* While a tweak is not overriding a property, follow Dia's own value so a
       later animation is what we restore. Once we write, the baseline stays. */
    if (!entry->over_vis && live->has_vis) {
        entry->has_base_vis = 1;
        entry->base_vis = live->vis;
    }
    if (!entry->over_margin && live->has_margin) {
        entry->has_base_margin = 1;
        memcpy(entry->base_margin, live->margin, sizeof entry->base_margin);
    }
    if (!entry->over_pad && live->has_pad) {
        entry->has_base_pad = 1;
        memcpy(entry->base_pad, live->pad, sizeof entry->base_pad);
    }
    if (!entry->over_rad && live->has_rad) {
        entry->has_base_rad = 1;
        memcpy(entry->base_rad, live->rad, sizeof entry->base_rad);
    }
    entry->have_base = 1;
}

static int close4(const double *a, const double *b) {
    for (int i = 0; i < 4; i++) {
        double d = a[i] - b[i];
        if (d < 0) d = -d;
        if (d > 0.05) return 0;
    }
    return 1;
}

static void quad_from(const double *v, Quad *q) {
    q->a = v[0];
    q->b = v[1];
    q->c = v[2];
    q->d = v[3];
}

static void fmt4(const double *v, char *out, int cap) {
    _snprintf(out, (size_t)cap, "%.1f,%.1f,%.1f,%.1f", v[0], v[1], v[2], v[3]);
}

static void apply_entry(Entry *entry, const TweakPlan *plan, const Live *live) {
    if (plan->visibility >= 0 && live->has_vis && live->vis != plan->visibility) {
        if (!g_settings.dry && write_vis(entry->ui, plan->visibility)) entry->over_vis = 1;
        if (!entry->logged_vis) {
            entry->logged_vis = 1;
            log_msg("%s visibility -> %s%s", entry->name[0] ? entry->name : "(unnamed)",
                 plan->visibility == 1 ? "collapsed" : "visible", g_settings.dry ? " (dry)" : "");
        }
    } else if (plan->visibility < 0 && entry->over_vis && entry->has_base_vis && live->has_vis &&
               live->vis != entry->base_vis) {
        if (!g_settings.dry && write_vis(entry->ui, entry->base_vis)) entry->over_vis = 0;
        if (!entry->logged_vis) {
            entry->logged_vis = 1;
            log_msg("%s visibility restored%s", entry->name, g_settings.dry ? " (dry)" : "");
        }
    }

    if (plan->setMargin && live->has_margin && !close4(live->margin, plan->margin)) {
        Quad q;
        quad_from(plan->margin, &q);
        if (!g_settings.dry && write_quad(entry->fe, VT_PUT_MARGIN, &q)) entry->over_margin = 1;
        if (!entry->logged_margin) {
            entry->logged_margin = 1;
            char before[64], after[64];
            fmt4(live->margin, before, sizeof before);
            fmt4(plan->margin, after, sizeof after);
            log_msg("%s margin %s -> %s%s", entry->name[0] ? entry->name : "(root)", before, after,
                 g_settings.dry ? " (dry)" : "");
        }
    } else if (!plan->setMargin && entry->over_margin && entry->has_base_margin && live->has_margin &&
               !close4(live->margin, entry->base_margin)) {
        Quad q;
        quad_from(entry->base_margin, &q);
        if (!g_settings.dry && write_quad(entry->fe, VT_PUT_MARGIN, &q)) entry->over_margin = 0;
        if (!entry->logged_margin) {
            entry->logged_margin = 1;
            log_msg("%s margin restored%s", entry->name[0] ? entry->name : "(root)", g_settings.dry ? " (dry)" : "");
        }
    }

    void *pad_obj = live->pad_border ? entry->border : entry->control;
    int pad_slot = live->pad_border ? VT_BORDER_PUT_PADDING : VT_CONTROL_PUT_PADDING;
    if (plan->setPadding && pad_obj && live->has_pad && !close4(live->pad, plan->padding)) {
        Quad q;
        quad_from(plan->padding, &q);
        if (!g_settings.dry && write_quad(pad_obj, pad_slot, &q)) entry->over_pad = 1;
        if (!entry->logged_pad) {
            entry->logged_pad = 1;
            log_msg("%s padding cleared%s", entry->name[0] ? entry->name : "(unnamed)", g_settings.dry ? " (dry)" : "");
        }
    } else if (!plan->setPadding && entry->over_pad && entry->has_base_pad && live->has_pad &&
               !close4(live->pad, entry->base_pad)) {
        Quad q;
        quad_from(entry->base_pad, &q);
        if (!g_settings.dry && write_quad(pad_obj, pad_slot, &q)) entry->over_pad = 0;
        if (!entry->logged_pad) {
            entry->logged_pad = 1;
            log_msg("%s padding restored%s", entry->name[0] ? entry->name : "(unnamed)", g_settings.dry ? " (dry)" : "");
        }
    }

    void *rad_obj = live->rad_border ? entry->border : entry->control;
    int rad_slot = live->rad_border ? VT_BORDER_PUT_RADIUS : VT_CONTROL_PUT_RADIUS;
    if (plan->setRadius && rad_obj && live->has_rad && !close4(live->rad, plan->radius)) {
        Quad q;
        quad_from(plan->radius, &q);
        if (!g_settings.dry && write_quad(rad_obj, rad_slot, &q)) entry->over_rad = 1;
        if (!entry->logged_rad) {
            entry->logged_rad = 1;
            log_msg("%s corner radius updated%s", entry->name[0] ? entry->name : "(unnamed)",
                 g_settings.dry ? " (dry)" : "");
        }
    } else if (!plan->setRadius && entry->over_rad && entry->has_base_rad && live->has_rad &&
               !close4(live->rad, entry->base_rad)) {
        Quad q;
        quad_from(entry->base_rad, &q);
        if (!g_settings.dry && write_quad(rad_obj, rad_slot, &q)) entry->over_rad = 0;
        if (!entry->logged_rad) {
            entry->logged_rad = 1;
            log_msg("%s corner radius restored%s", entry->name[0] ? entry->name : "(unnamed)",
                 g_settings.dry ? " (dry)" : "");
        }
    }
}

static int local_appdata(wchar_t *out, int cap) {
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", out, cap);
    if (n > 0 && n < (DWORD)cap) return 1;
    PWSTR known = NULL;
    if (SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &known) != S_OK || !known) return 0;
    _snwprintf(out, cap, L"%s", known);
    CoTaskMemFree(known);
    return out[0] != 0;
}

static void load_settings(void) {
    wchar_t root[MAX_PATH], path[MAX_PATH];
    if (!local_appdata(root, MAX_PATH)) return;
    _snwprintf(path, MAX_PATH, L"%s\\DiaPatches\\tweaks.json", root);
    FILE *file = _wfopen(path, L"rb");
    if (!file) {
        if (g_settings_text[0]) {
            tweak_defaults(&g_settings);
            g_settings_text[0] = 0;
            log_line("settings file removed; tweaks off");
        }
        return;
    }
    char buf[sizeof g_settings_text];
    size_t n = fread(buf, 1, sizeof buf - 1, file);
    fclose(file);
    buf[n] = 0;
    if (strcmp(buf, g_settings_text) == 0) return;
    memcpy(g_settings_text, buf, n + 1);
    tweak_parse(buf, &g_settings);
    for (int i = 0; i < g_ncache; i++) {
        g_cache[i].logged_vis = g_cache[i].logged_margin = g_cache[i].logged_pad = g_cache[i].logged_rad = 0;
    }
    log_msg("settings edge=%d square=%d toolbar=%d sidebar=%d chat=%d dry=%d inset=%.1f", g_settings.edgeToEdge,
         g_settings.squareSidebarCorners, g_settings.hideToolbar, g_settings.hideSidebarButton, g_settings.hideChatButton,
         g_settings.dry, g_settings.inset);
}

static int dia_file(const wchar_t *name, wchar_t *out, int cap) {
    wchar_t root[MAX_PATH];
    if (!local_appdata(root, MAX_PATH)) return 0;
    return _snwprintf(out, cap, L"%s\\DiaPatches\\%s", root, name) > 0;
}

static int file_time(const wchar_t *path, FILETIME *out) {
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &info)) return 0;
    *out = info.ftLastWriteTime;
    return 1;
}

static void remember_json(void) {
    wchar_t path[MAX_PATH];
    g_json_seen_ok = dia_file(L"tweaks.json", path, MAX_PATH) && file_time(path, &g_json_seen);
}

static int json_changed(void) {
    wchar_t path[MAX_PATH];
    FILETIME now;
    if (!g_json_seen_ok || !dia_file(L"tweaks.json", path, MAX_PATH) || !file_time(path, &now)) return 0;
    return CompareFileTime(&now, &g_json_seen) != 0;
}

static int json_newer_than_log(void) {
    wchar_t json[MAX_PATH], log_path[MAX_PATH];
    FILETIME json_time, log_time;
    if (!dia_file(L"tweaks.json", json, MAX_PATH) || !dia_file(L"diatweaks.log", log_path, MAX_PATH)) return 0;
    if (!file_time(json, &json_time) || !file_time(log_path, &log_time)) return 0;
    return CompareFileTime(&json_time, &log_time) > 0;
}

static int previous_run_crashed(void) {
    wchar_t path[MAX_PATH];
    FILE *file;
    long size, take;
    char *buf;
    size_t n;
    int crashed;
    if (!dia_file(L"diatweaks.log", path, MAX_PATH)) return 0;
    file = _wfopen(path, L"rb");
    if (!file) return 0;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 0;
    }
    size = ftell(file);
    if (size <= 0) {
        fclose(file);
        return 0;
    }
    take = size > 8192 ? 8192 : size;
    if (fseek(file, -take, SEEK_END) != 0) {
        fclose(file);
        return 0;
    }
    buf = (char *)malloc((size_t)take + 1);
    if (!buf) {
        fclose(file);
        return 0;
    }
    n = fread(buf, 1, (size_t)take, file);
    fclose(file);
    buf[n] = 0;
    crashed = tweak_log_crashed(buf);
    free(buf);
    return crashed;
}

static void fault_trampoline(void) { longjmp(g_abort, 1); }

static LONG CALLBACK discovery_fault(EXCEPTION_POINTERS *info) {
    DWORD code;
    if (!g_guard || !info || !info->ExceptionRecord || !info->ContextRecord) return EXCEPTION_CONTINUE_SEARCH;
    code = info->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_IN_PAGE_ERROR && code != EXCEPTION_ILLEGAL_INSTRUCTION)
        return EXCEPTION_CONTINUE_SEARCH;
    g_guard = 0;
    g_fault_code = code;
    if (g_log_held) {
        g_log_held = 0;
        LeaveCriticalSection(&g_log_lock);
    }
    /* Enter fault_trampoline as if CALL pushed a return address: RSP is 8 mod 16. */
    info->ContextRecord->Rip = (DWORD64)(uintptr_t)fault_trampoline;
    info->ContextRecord->Rsp &= ~(DWORD64)15;
    info->ContextRecord->Rsp -= 8;
    return EXCEPTION_CONTINUE_EXECUTION;
}

static void discover(void) {
    PVOID veh = AddVectoredExceptionHandler(1, discovery_fault);
    g_guard = 1;
    if (setjmp(g_abort) == 0) {
        rebuild();
        g_guard = 0;
        log_line("discovery finished");
    } else {
        g_guard = 0;
        g_disabled = 1;
        g_have_root = 0;
        remember_json();
        log_msg("discovery fault during %s code=%08lx; add-on disabled", g_step ? g_step : "unknown",
                (unsigned long)g_fault_code);
    }
    if (veh) RemoveVectoredExceptionHandler(veh);
}

static int toplevels_changed(void) {
    WinList tops = {GetCurrentProcessId(), {0}, 0};
    int changed = 0;
    EnumWindows(collect_top, (LPARAM)&tops);
    if (tops.count != g_known_top_n) changed = 1;
    for (int i = 0; !changed && i < tops.count; i++) {
        int found = 0;
        for (int j = 0; j < g_known_top_n; j++)
            if (g_known_tops[j] == tops.hwnds[i]) found = 1;
        if (!found) changed = 1;
    }
    g_known_top_n = tops.count;
    for (int i = 0; i < tops.count && i < 16; i++) g_known_tops[i] = tops.hwnds[i];
    return changed;
}

static void apply(void) {
    DWORD now;
    if (!g_host_ok || !ensure_sta()) return;
    load_settings();
    if (g_disabled) {
        if (!json_changed()) return;
        g_disabled = 0;
        g_discover_paused = 0;
        g_discover_attempts = 0;
        g_discover_gap = 0;
        g_discover_start = 0;
        remember_json();
        log_line("settings re-saved; discovery enabled");
    }
    now = GetTickCount();
    if (g_have_root) {
        if ((g_tick++ % 5) == 0) {
            g_detail = 0;
            discover();
        }
    } else if (g_discover_paused) {
        if (toplevels_changed()) {
            log_line("new top-level window; retrying discovery");
            g_discover_paused = 0;
            g_discover_attempts = 0;
            g_discover_gap = 1000;
            g_discover_start = now;
            g_detail = 1;
            discover();
        }
    } else if (g_discover_attempts == 0 || now - g_last_discover >= g_discover_gap) {
        g_detail = g_discover_attempts == 0;
        g_discover_attempts++;
        g_last_discover = now;
        if (!g_discover_start) g_discover_start = now;
        discover();
        if (!g_have_root && !g_disabled) {
            if (g_discover_gap < 1000) g_discover_gap = 1000;
            else if (g_discover_gap < 30000) g_discover_gap *= 2;
            if (g_discover_gap > 30000) g_discover_gap = 30000;
            if (g_discover_attempts == 8 || now - g_discover_start > 180000) {
                toplevels_changed();
                g_discover_paused = 1;
                log_msg("discovery paused after %d attempts; waiting for a new top-level window", g_discover_attempts);
            } else if (!g_detail) {
                log_msg("discovery attempt %d failed; next in %lu ms", g_discover_attempts, g_discover_gap);
            }
        }
    }

    Live lives[MAX_CACHE];
    int assistant = 0;
    for (int i = 0; i < g_ncache; i++) {
        read_live(&g_cache[i], &lives[i]);
        if (!lives[i].ok) continue;
        track_baseline(&g_cache[i], &lives[i]);
        if (strcmp(g_cache[i].name, "AssistantPanelHost") == 0 && lives[i].has_vis && lives[i].vis == 0 &&
            lives[i].width > 24)
            assistant = 1;
    }

    TweakPlan plans[MAX_CACHE];
    int depth[MAX_CACHE], tree[MAX_CACHE];
    for (int i = 0; i < g_ncache; i++) {
        depth[i] = g_cache[i].depth;
        tree[i] = g_cache[i].tree;
        TweakElement element;
        memset(&element, 0, sizeof element);
        element.name = g_cache[i].name;
        element.parentName = g_cache[i].parent;
        element.assistantOpen = assistant;
        element.hasMargin = g_cache[i].has_base_margin;
        element.hasPadding = g_cache[i].has_base_pad;
        element.hasRadius = g_cache[i].has_base_rad;
        memcpy(element.margin, g_cache[i].base_margin, sizeof element.margin);
        memcpy(element.padding, g_cache[i].base_pad, sizeof element.padding);
        memcpy(element.radius, g_cache[i].base_rad, sizeof element.radius);
        element.hasPlace = lives[i].has_place;
        element.offsetX = lives[i].offset_x;
        element.width = lives[i].width > 0 ? lives[i].width : 0;
        element.height = lives[i].height;
        element.parentWidth = lives[i].parent_width;
        /* Baseline width is what inset_host uses for the unnamed root. */
        if (g_cache[i].have_base && element.width < 200) {
            element.width = lives[i].width;
            element.height = lives[i].height;
        }
        tweak_plan(&g_settings, &element, &plans[i]);
    }
    tweak_prefer_outer_inset(plans, depth, tree, g_ncache);
    if (g_dumped) {
        for (int i = 0; i < g_ncache; i++) {
            if (!lives[i].ok || !tweak_plan_changes(&plans[i])) continue;
            log_msg("plan %s visibility=%d margin=%d padding=%d radius=%d",
                    g_cache[i].name[0] ? g_cache[i].name : "(unnamed)", plans[i].visibility, plans[i].setMargin,
                    plans[i].setPadding, plans[i].setRadius);
        }
    }
    for (int i = 0; i < g_ncache; i++)
        if (lives[i].ok) apply_entry(&g_cache[i], &plans[i], &lives[i]);
}

static void CALLBACK timer_proc(HWND hwnd, UINT msg, UINT_PTR id, DWORD time) {
    (void)msg;
    (void)id;
    (void)time;
    if (!IsWindow(hwnd)) return;
    static int busy;
    if (busy) return;
    busy = 1;
    apply();
    busy = 0;
}

static LRESULT CALLBACK hook_proc(int code, WPARAM wparam, LPARAM lparam) {
    int done = 0;
    if (code == HC_ACTION) {
        CWPSTRUCT *msg = (CWPSTRUCT *)lparam;
        if (msg->message == WM_NULL && msg->hwnd == g_pending && !g_timer_hwnd) {
            if (SetTimer(msg->hwnd, 0xD1A7, 200, timer_proc)) g_timer_hwnd = msg->hwnd;
            done = 1;
        }
    }
    LRESULT result = CallNextHookEx(NULL, code, wparam, lparam);
    if (done && g_hook) {
        HHOOK hook = g_hook;
        g_hook = NULL;
        UnhookWindowsHookEx(hook);
    }
    return result;
}

static BOOL CALLBACK pick_cb(HWND hwnd, LPARAM param) {
    HWND *best = (HWND *)param;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != GetCurrentProcessId() || !IsWindowVisible(hwnd)) return TRUE;
    RECT rect;
    if (!GetWindowRect(hwnd, &rect)) return TRUE;
    LONG area = (rect.right - rect.left) * (rect.bottom - rect.top);
    if (area < 80 * 80) return TRUE;
    RECT cur = {0};
    LONG cur_area = 0;
    if (*best && GetWindowRect(*best, &cur)) cur_area = (cur.right - cur.left) * (cur.bottom - cur.top);
    if (area > cur_area) *best = hwnd;
    return TRUE;
}

static DWORD WINAPI worker(void *unused) {
    (void)unused;
    if (previous_run_crashed()) {
        if (json_newer_than_log()) log_line("settings re-saved; discovery enabled");
        else {
            g_disabled = 1;
            remember_json();
            log_line("previous run crashed; disabled");
        }
    } else {
        log_line("loaded");
    }
    for (;;) {
        Sleep(500);
        if (g_timer_hwnd && IsWindow(g_timer_hwnd)) continue;
        g_timer_hwnd = NULL;
        HWND best = NULL;
        EnumWindows(pick_cb, (LPARAM)&best);
        if (!best) continue;
        DWORD tid = GetWindowThreadProcessId(best, NULL);
        g_pending = best;
        g_hook = SetWindowsHookExW(WH_CALLWNDPROC, hook_proc, g_module, tid);
        if (!g_hook) {
            log_msg("could not reach the UI thread (%lu)", GetLastError());
            Sleep(2000);
            continue;
        }
        DWORD_PTR ignored = 0;
        if (!SendMessageTimeoutW(best, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, 2000, &ignored) && g_hook) {
            UnhookWindowsHookEx(g_hook);
            g_hook = NULL;
        }
    }
    return 0;
}

static int contains_i(const wchar_t *hay, const wchar_t *needle) {
    size_t n = wcslen(needle);
    for (; *hay; hay++)
        if (_wcsnicmp(hay, needle, n) == 0) return 1;
    return 0;
}

static int host_is_dia(void) {
    wchar_t path[2048];
    DWORD n = GetModuleFileNameW(NULL, path, 2048);
    if (!n || n >= 2048) return 0;
    const wchar_t *base = wcsrchr(path, L'\\');
    base = base ? base + 1 : path;
    if (_wcsicmp(base, L"Dia.exe") != 0) return 0;
    if (!contains_i(path, L"\\WindowsApps\\TheBrowserCompany.Dia")) return 0;
    /* Chromium utility processes are also Dia.exe. They have no WinUI tree. */
    wchar_t *cmd = GetCommandLineW();
    return !(cmd && wcsstr(cmd, L"--type="));
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_DETACH) {
        /* A disabled run must not end on "shutdown", or the next launch would try discovery again. */
        if (g_host_ok && !g_disabled) log_line("shutdown");
        return TRUE;
    }
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    g_module = module;
    DisableThreadLibraryCalls(module);
    InitializeCriticalSection(&g_log_lock);
    tweak_defaults(&g_settings);
    g_host_ok = host_is_dia();
    if (!g_host_ok) return TRUE;
    CreateThread(NULL, 0, worker, NULL, 0, NULL);
    return TRUE;
}
