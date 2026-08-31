#pragma once

namespace qml {

/**
 * Loads every font shipped in the application's fonts directory into the font database.
 *
 * The design's type stack -- Barlow, Barlow Condensed and JetBrains Mono -- is vendored rather than
 * assumed present on the machine, so Theme.qml can name those families outright. Must run before
 * the QML engine loads, and before anything measures text: GraphViewStyle sizes node boxes from
 * real font metrics, and a missing family silently falls back to a different one with different
 * advances.
 */
void loadApplicationFonts();

}    // namespace qml
