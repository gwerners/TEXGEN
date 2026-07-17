#pragma once
// Category icons for the node creation menu, rasterized from embedded
// SVGs with NanoSVG at startup (lazily, once a GL context exists).

#include <raylib.h>
#include <string>

// Returns the icon texture for a node category ("Generator", "Filter"...)
// or nullptr if the category has no icon. Textures are cached for the
// lifetime of the process (freed by the GL context teardown).
Texture2D* categoryIcon(const std::string& category);

// Returns a toolbar/UI icon texture by name ("new", "open", "save",
// "saveas", "import", "export", "play", "refresh") or nullptr.
Texture2D* uiIcon(const std::string& name);
