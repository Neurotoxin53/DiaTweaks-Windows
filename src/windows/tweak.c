#include "tweak.h"

#include <ctype.h>
#include <string.h>

static const char *kContent[] = {"ContentGroup", "ContentHost", "VariantHost", NULL};

static int same(const char *a, const char *b) { return strcmp(a, b) == 0; }

static int one_of(const char *name, const char *const *list) {
    for (int i = 0; list[i]; i++)
        if (same(name, list[i])) return 1;
    return 0;
}

static int is_content(const char *name) { return one_of(name, kContent); }

static const char *skip_space(const char *p) {
    while (*p && isspace((unsigned char)*p)) p++;
    return p;
}

/* p points just past the opening quote of a key. */
static int key_is(const char *p, const char *key, const char **after) {
    size_t n = strlen(key);
    if (strncmp(p, key, n) != 0 || p[n] != '"') return 0;
    *after = p + n + 1;
    return 1;
}

static int take_word(const char *p, const char *word) {
    size_t n = strlen(word);
    return strncmp(p, word, n) == 0 && !isalnum((unsigned char)p[n]);
}

static int take_double(const char *p, double *out, const char **end) {
    const char *s = p;
    if (*s == '+') s++;
    if (!isdigit((unsigned char)*s)) return 0;
    double v = 0;
    while (isdigit((unsigned char)*s)) {
        v = v * 10 + (*s - '0');
        s++;
    }
    if (*s == '.') {
        s++;
        double place = 0.1;
        if (!isdigit((unsigned char)*s)) return 0;
        while (isdigit((unsigned char)*s)) {
            v += (*s - '0') * place;
            place *= 0.1;
            s++;
        }
    }
    *out = v;
    *end = s;
    return 1;
}

void tweak_defaults(TweakSettings *settings) {
    settings->edgeToEdge = 0;
    settings->squareSidebarCorners = 0;
    settings->hideToolbar = 0;
    settings->hideSidebarButton = 0;
    settings->hideChatButton = 0;
    settings->dry = 0;
    settings->inset = 6;
}

void tweak_parse(const char *text, TweakSettings *settings) {
    tweak_defaults(settings);
    if (!text) return;
    if ((unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF) text += 3;
    const char *p = text;
    while ((p = strchr(p, '"')) != NULL) {
        p++;
        const char *after = NULL;
        int *flag = NULL;
        if (key_is(p, "edgeToEdge", &after)) flag = &settings->edgeToEdge;
        else if (key_is(p, "squareSidebarCorners", &after)) flag = &settings->squareSidebarCorners;
        else if (key_is(p, "hideToolbar", &after)) flag = &settings->hideToolbar;
        else if (key_is(p, "hideSidebarButton", &after)) flag = &settings->hideSidebarButton;
        else if (key_is(p, "hideChatButton", &after)) flag = &settings->hideChatButton;
        else if (key_is(p, "dry", &after)) flag = &settings->dry;
        else if (key_is(p, "inset", &after)) {
            after = skip_space(after);
            if (*after != ':') {
                p = after;
                continue;
            }
            after = skip_space(after + 1);
            double v = 0;
            const char *end = after;
            if (take_double(after, &v, &end) && v <= 64) settings->inset = v;
            p = end;
            continue;
        } else {
            continue;
        }
        after = skip_space(after);
        if (*after != ':') {
            p = after;
            continue;
        }
        after = skip_space(after + 1);
        if (take_word(after, "true")) *flag = 1;
        else if (take_word(after, "false")) *flag = 0;
        p = after;
    }
}

int tweak_interesting(const TweakElement *element) {
    if (same(element->name, "SidebarToggleButton") || same(element->name, "ChatButton") ||
        same(element->name, "NavigationBarHost") || same(element->name, "AssistantPanelHost"))
        return 1;
    if (is_content(element->name) || is_content(element->parentName)) return 1;
    if (element->name[0] == 0 && element->parentName[0] == 0) return 1;
    if (element->hasRadius && element->width >= 200 && element->height >= 200) return 1;
    return 0;
}

static int near_inset(double value, double inset) {
    double d = value - inset;
    if (d < 0) d = -d;
    return d <= 0.5;
}

static void zero_inset_sides(const double *in, double inset, double *out, int *any) {
    for (int i = 0; i < 4; i++) {
        out[i] = in[i];
        if (near_inset(in[i], inset)) {
            out[i] = 0;
            *any = 1;
        }
    }
}

static int inset_host(const TweakElement *element, double inset) {
    if (is_content(element->name)) return 1;
    if (is_content(element->parentName) && element->name[0] == 0) return 1;
    if (element->name[0] != 0 || element->parentName[0] != 0 || !element->hasMargin || element->width < 200 ||
        element->height < 200)
        return 0;
    for (int i = 0; i < 4; i++)
        if (!near_inset(element->margin[i], inset)) return 0;
    return 1;
}

void tweak_plan(const TweakSettings *settings, const TweakElement *element, TweakPlan *plan) {
    memset(plan, 0, sizeof *plan);
    plan->visibility = -1;
    if (same(element->name, "SidebarToggleButton") && settings->hideSidebarButton) plan->visibility = 1;
    if (same(element->name, "ChatButton") && settings->hideChatButton) plan->visibility = 1;
    if (same(element->name, "NavigationBarHost") && settings->hideToolbar) plan->visibility = 1;

    int pull = settings->edgeToEdge && !element->assistantOpen && settings->inset > 0 && inset_host(element, settings->inset);
    if (pull && element->hasMargin) {
        int any = 0;
        zero_inset_sides(element->margin, settings->inset, plan->margin, &any);
        if (any) plan->setMargin = 1;
    }
    if (pull && element->hasPadding) {
        int any = 0;
        zero_inset_sides(element->padding, settings->inset, plan->padding, &any);
        if (any) plan->setPadding = 1;
    }

    int square = settings->edgeToEdge && settings->squareSidebarCorners && !element->assistantOpen;
    if (!square || !element->hasRadius || !element->hasPlace || element->parentWidth <= 0 || element->width < 200 ||
        element->height < 200)
        return;
    for (int i = 0; i < 4; i++)
        if (element->radius[i] < 0 || element->radius[i] > 32) return;
    double gapL = element->offsetX;
    double gapR = element->parentWidth - element->offsetX - element->width;
    int left = gapL > settings->inset + 1;
    int right = gapR > settings->inset + 1;
    if (left == right) return;
    for (int i = 0; i < 4; i++) plan->radius[i] = element->radius[i];
    if (left) {
        plan->radius[0] = 0;
        plan->radius[3] = 0;
    } else {
        plan->radius[1] = 0;
        plan->radius[2] = 0;
    }
    for (int i = 0; i < 4; i++)
        if (plan->radius[i] != element->radius[i]) plan->setRadius = 1;
}

void tweak_prefer_outer_inset(TweakPlan *plans, const int *depth, const int *tree, int count) {
    int seen[32], best[32], nseen = 0;
    for (int i = 0; i < count; i++) {
        if (!plans[i].setMargin && !plans[i].setPadding) continue;
        int slot = -1;
        for (int s = 0; s < nseen; s++)
            if (seen[s] == tree[i]) slot = s;
        if (slot < 0) {
            if (nseen == 32) continue;
            slot = nseen++;
            seen[slot] = tree[i];
            best[slot] = depth[i];
        } else if (depth[i] < best[slot]) {
            best[slot] = depth[i];
        }
    }
    for (int i = 0; i < count; i++) {
        if (!plans[i].setMargin && !plans[i].setPadding) continue;
        int keep = 0;
        for (int s = 0; s < nseen; s++)
            if (seen[s] == tree[i] && depth[i] == best[s]) keep = 1;
        if (!keep) {
            plans[i].setMargin = 0;
            plans[i].setPadding = 0;
        }
    }
}

int tweak_plan_changes(const TweakPlan *plan) {
    return plan->visibility >= 0 || plan->setMargin || plan->setPadding || plan->setRadius;
}
