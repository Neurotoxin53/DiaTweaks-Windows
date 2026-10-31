#include "tweak.h"

#include <stdio.h>
#include <string.h>

static int g_failed;

#define CHECK(cond)                                                                                                    \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            fprintf(stderr, "fail %s:%d %s\n", __FILE__, __LINE__, #cond);                                            \
            g_failed++;                                                                                                \
        }                                                                                                              \
    } while (0)

static TweakElement blank(const char *name, const char *parent) {
    TweakElement element;
    memset(&element, 0, sizeof element);
    element.name = name;
    element.parentName = parent;
    return element;
}

static void test_parse(void) {
    const char *text =
        "{\n"
        "  \"edgeToEdge\": true,\n"
        "  \"squareSidebarCorners\": false,\n"
        "  \"hideToolbar\": false,\n"
        "  \"hideSidebarButton\": true,\n"
        "  \"hideChatButton\": false,\n"
        "  \"dry\": true,\n"
        "  \"inset\": 6.5\n"
        "}\n";
    TweakSettings settings;
    tweak_parse(text, &settings);
    CHECK(settings.edgeToEdge == 1);
    CHECK(settings.squareSidebarCorners == 0);
    CHECK(settings.hideToolbar == 0);
    CHECK(settings.hideSidebarButton == 1);
    CHECK(settings.hideChatButton == 0);
    CHECK(settings.dry == 1);
    CHECK(settings.inset > 6.49 && settings.inset < 6.51);

    unsigned char bom[] = {0xEF, 0xBB, 0xBF, '"', 'h', 'i', 'd', 'e', 'T', 'o', 'o', 'l', 'b', 'a', 'r', '"', ':', ' ',
                           't', 'r', 'u', 'e', 0};
    tweak_parse((char *)bom, &settings);
    CHECK(settings.hideToolbar == 1);
    CHECK(settings.edgeToEdge == 0);

    tweak_parse("{\"inset\": 80}", &settings);
    CHECK(settings.inset == 6);
    tweak_parse("{\"edgeToEdges\": true}", &settings);
    CHECK(settings.edgeToEdge == 0);
    tweak_parse(NULL, &settings);
    CHECK(settings.hideChatButton == 0);
}

static void test_buttons(void) {
    TweakSettings settings;
    tweak_defaults(&settings);
    TweakElement button = blank("SidebarToggleButton", "NavigationBarContent");
    TweakPlan plan;
    tweak_plan(&settings, &button, &plan);
    CHECK(plan.visibility == -1);
    settings.hideSidebarButton = 1;
    tweak_plan(&settings, &button, &plan);
    CHECK(plan.visibility == 1);
    CHECK(!plan.setMargin);

    TweakElement chat = blank("ChatButton", "NavigationBarContent");
    settings.hideToolbar = 1;
    tweak_plan(&settings, &chat, &plan);
    CHECK(plan.visibility == -1);
    settings.hideChatButton = 1;
    tweak_plan(&settings, &chat, &plan);
    CHECK(plan.visibility == 1);

    TweakElement bar = blank("NavigationBarHost", "ContentStack");
    tweak_plan(&settings, &bar, &plan);
    CHECK(plan.visibility == 1);
    settings.hideToolbar = 0;
    tweak_plan(&settings, &bar, &plan);
    CHECK(plan.visibility == -1);
}

static void test_inset(void) {
    TweakSettings settings;
    tweak_defaults(&settings);
    settings.edgeToEdge = 1;
    TweakElement host = blank("ContentHost", "ContentGroup");
    host.hasMargin = 1;
    host.margin[0] = host.margin[1] = host.margin[2] = host.margin[3] = 6;
    host.width = 800;
    host.height = 600;
    TweakPlan plan;
    tweak_plan(&settings, &host, &plan);
    CHECK(plan.setMargin == 1);
    CHECK(plan.margin[0] == 0 && plan.margin[1] == 0 && plan.margin[2] == 0 && plan.margin[3] == 0);

    host.margin[0] = 24;
    host.margin[1] = 6;
    host.margin[2] = 24;
    host.margin[3] = 6;
    tweak_plan(&settings, &host, &plan);
    CHECK(plan.setMargin == 1);
    CHECK(plan.margin[0] == 24 && plan.margin[1] == 0 && plan.margin[2] == 24 && plan.margin[3] == 0);

    host.margin[1] = 6.6;
    host.margin[3] = 6.6;
    tweak_plan(&settings, &host, &plan);
    CHECK(plan.setMargin == 0);

    host.margin[0] = host.margin[1] = host.margin[2] = host.margin[3] = 6;
    host.assistantOpen = 1;
    tweak_plan(&settings, &host, &plan);
    CHECK(plan.setMargin == 0);

    settings.edgeToEdge = 0;
    host.assistantOpen = 0;
    tweak_plan(&settings, &host, &plan);
    CHECK(plan.setMargin == 0);

    settings.edgeToEdge = 1;
    TweakElement root = blank("", "");
    root.hasMargin = 1;
    root.margin[0] = root.margin[1] = root.margin[2] = root.margin[3] = 6;
    root.width = 900;
    root.height = 700;
    settings.inset = 6;
    tweak_plan(&settings, &root, &plan);
    CHECK(plan.setMargin == 1);
    CHECK(plan.margin[0] == 0 && plan.margin[3] == 0);

    settings.edgeToEdge = 1;
    settings.inset = 8;
    host.hasPadding = 1;
    host.padding[0] = host.padding[1] = host.padding[2] = host.padding[3] = 8;
    host.hasMargin = 0;
    tweak_plan(&settings, &host, &plan);
    CHECK(plan.setPadding == 1);
    CHECK(plan.padding[0] == 0 && plan.padding[2] == 0);
}

static void test_square(void) {
    TweakSettings settings;
    tweak_defaults(&settings);
    settings.edgeToEdge = 1;
    settings.squareSidebarCorners = 1;
    TweakElement card = blank("PageBorder", "ContentHost");
    card.hasRadius = 1;
    card.radius[0] = card.radius[1] = card.radius[2] = card.radius[3] = 12;
    card.hasPlace = 1;
    card.offsetX = 240;
    card.width = 756;
    card.height = 700;
    card.parentWidth = 1000;
    TweakPlan plan;
    tweak_plan(&settings, &card, &plan);
    CHECK(plan.setRadius == 1);
    CHECK(plan.radius[0] == 0 && plan.radius[3] == 0);
    CHECK(plan.radius[1] == 12 && plan.radius[2] == 12);

    card.offsetX = 2;
    card.width = 700;
    tweak_plan(&settings, &card, &plan);
    CHECK(plan.setRadius == 1);
    CHECK(plan.radius[1] == 0 && plan.radius[2] == 0);
    CHECK(plan.radius[0] == 12 && plan.radius[3] == 12);

    card.offsetX = 100;
    card.width = 400;
    tweak_plan(&settings, &card, &plan);
    CHECK(plan.setRadius == 0);

    card.offsetX = 240;
    card.width = 756;
    card.parentWidth = 0;
    tweak_plan(&settings, &card, &plan);
    CHECK(plan.setRadius == 0);

    card.parentWidth = 1000;
    settings.squareSidebarCorners = 0;
    tweak_plan(&settings, &card, &plan);
    CHECK(plan.setRadius == 0);

    settings.squareSidebarCorners = 1;
    settings.edgeToEdge = 0;
    tweak_plan(&settings, &card, &plan);
    CHECK(plan.setRadius == 0);

    settings.edgeToEdge = 1;
    card.radius[0] = 40;
    tweak_plan(&settings, &card, &plan);
    CHECK(plan.setRadius == 0);

    card.radius[0] = 12;
    card.assistantOpen = 1;
    tweak_plan(&settings, &card, &plan);
    CHECK(plan.setRadius == 0);
}

static void test_outer(void) {
    TweakPlan plans[3];
    memset(plans, 0, sizeof plans);
    plans[0].setMargin = 1;
    plans[1].setPadding = 1;
    plans[1].setRadius = 1;
    plans[2].setMargin = 1;
    int depth[] = {1, 4, 1};
    int tree[] = {7, 7, 8};
    tweak_prefer_outer_inset(plans, depth, tree, 3);
    CHECK(plans[0].setMargin == 1);
    CHECK(plans[1].setPadding == 0);
    CHECK(plans[1].setRadius == 1);
    CHECK(plans[2].setMargin == 1);
}

static void test_log_tail(void) {
    CHECK(tweak_log_crashed(NULL) == 0);
    CHECK(tweak_log_crashed("") == 0);
    CHECK(tweak_log_crashed("\n") == 0);
    CHECK(tweak_log_crashed("2026-10-31 22:04:17 discovery finished\n") == 0);
    CHECK(tweak_log_crashed("2026-10-31 22:04:17 shutdown\r\n") == 0);
    CHECK(tweak_log_crashed("22:04:17 loaded\n22:04:17 Window.Current hr=00000000 null\n") == 1);
    CHECK(tweak_log_crashed("2026-10-31 22:04:17 discovery finished\n2026-10-31 22:04:18 step island-appdata") == 1);
    CHECK(tweak_log_crashed("2026-10-31 22:04:17 previous run crashed; disabled\n") == 1);
    CHECK(tweak_log_crashed("2026-10-31 22:04:17 discovery fault during island-appdata; add-on disabled\n") == 1);
    CHECK(tweak_log_crashed("2026-10-31 22:03:54 settings edge=1\n2026-10-31 22:04:17 shutdown\n") == 0);
}

static void test_interesting(void) {
    TweakElement root = blank("", "");
    CHECK(tweak_interesting(&root) == 1);
    TweakElement page = blank("VariantHost", "ContentStack");
    CHECK(tweak_interesting(&page) == 1);
    TweakElement label = blank("StatusPillText", "NavigationBarContent");
    label.width = 40;
    label.height = 16;
    CHECK(tweak_interesting(&label) == 0);
}

int main(void) {
    test_parse();
    test_buttons();
    test_inset();
    test_square();
    test_outer();
    test_log_tail();
    test_interesting();
    if (g_failed) {
        fprintf(stderr, "%d failed\n", g_failed);
        return 1;
    }
    printf("ok\n");
    return 0;
}
