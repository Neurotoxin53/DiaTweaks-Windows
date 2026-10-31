#ifndef DIATWEAKS_TWEAK_H
#define DIATWEAKS_TWEAK_H

/* Decisions for Dia's WinUI tree. The macOS add-on hooks AppKit views; this
   module only decides what to change. It does not call Windows. */

typedef struct TweakSettings {
    int edgeToEdge;
    int squareSidebarCorners;
    int hideToolbar;
    int hideSidebarButton;
    int hideChatButton;
    int dry;
    double inset; /* DIP. A side within 0.5 of this is the window padding. */
} TweakSettings;

typedef struct TweakElement {
    const char *name;       /* never null */
    const char *parentName; /* never null */
    int assistantOpen;
    int hasMargin;
    double margin[4]; /* left, top, right, bottom */
    int hasPadding;
    double padding[4];
    int hasRadius;
    double radius[4]; /* top-left, top-right, bottom-right, bottom-left */
    int hasPlace;
    double offsetX; /* child's offset inside its parent, DIP */
    double width;
    double height;
    double parentWidth;
} TweakElement;

typedef struct TweakPlan {
    int visibility; /* -1 leave it, 1 collapsed */
    int setMargin;
    double margin[4];
    int setPadding;
    double padding[4];
    int setRadius;
    double radius[4];
} TweakPlan;

void tweak_defaults(TweakSettings *settings);
/* Unknown keys are ignored. inset is kept only when it is between 0 and 64. */
void tweak_parse(const char *text, TweakSettings *settings);
int tweak_interesting(const TweakElement *element);
void tweak_plan(const TweakSettings *settings, const TweakElement *element, TweakPlan *plan);
/* Drop inset edits on deeper elements of the same tree so a parent and its
   child are not both pulled outward. tree[] and depth[] are parallel to plans. */
void tweak_prefer_outer_inset(TweakPlan *plans, const int *depth, const int *tree, int count);
int tweak_plan_changes(const TweakPlan *plan);

#endif
