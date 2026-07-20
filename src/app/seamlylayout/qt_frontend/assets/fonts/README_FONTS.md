# Fonts for SeamlyLayout

This directory should contain the following Google Fonts:

## Required Fonts

### Comfortaa
- **Comfortaa-Regular.ttf** - Regular weight (400)
- **Comfortaa-Bold.ttf** - Bold weight (700)
- **Comfortaa-Light.ttf** - Light weight (300)

### Inter Tight
- **InterTight-Regular.ttf** - Regular weight (400)
- **InterTight-Bold.ttf** - Bold weight (700)
- **InterTight-Light.ttf** - Light weight (300)

## How to Download

1. Visit [Google Fonts](https://fonts.google.com/)
2. Search for "Comfortaa" and "Inter Tight"
3. Download the font files
4. Place them in this directory

## Font Pairing Strategy

- **Comfortaa**: Used for headings, titles, and branded UI elements
- **Inter Tight**: Used for body text, dense forms, and data-heavy layouts

This pairing provides excellent readability and a modern, professional appearance suitable for a design tool like SeamlyLayout.

## Alternative: Using Google Fonts Package

If you prefer to use the `google_fonts` package instead of local fonts, you can:

1. Add `google_fonts: ^6.1.0` to pubspec.yaml dependencies
2. Update the theme to use `GoogleFonts.comfortaa()` and `GoogleFonts.interTight()`
3. Remove the local font files from this directory
