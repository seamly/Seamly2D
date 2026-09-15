message(" ")
message("===== Entering macos.pro =====")
message(" ")

TEMPLATE = aux

macx{
    APPLE_SIGN_IDENTITY = $$shell_quote($(APPLE_SIGN_IDENTITY))
    APPLE_NOTARIZE_KEY = $(APPLE_NOTARIZE_KEY)
    APPLE_NOTARIZE_KEY_ID = $(APPLE_NOTARIZE_KEY_ID)
    APPLE_NOTARIZE_ISSUER_ID = $(APPLE_NOTARIZE_ISSUER_ID)

    # SeamlyLayout.app is built by CMake/Cargo before qmake runs (see ci.yml's
    # 'Build SeamlyLayout release' step) — this rule packages that bundle
    # alongside the qmake-built Seamly2D and SeamlyME bundles into one DMG.
    seamlysuitedmg.target = Seamly2D-macos.dmg
    seamlysuitedmg.commands = mkdir -p $${OUT_PWD}
    seamlysuitedmg.commands += && hdiutil create \
        -srcfolder $${OUT_PWD}/../../src/app/seamly2d/bin/Seamly2D.app \
        -srcfolder $${OUT_PWD}/../../src/app/seamlyme/bin/seamlyme.app \
        -srcfolder $${OUT_PWD}/../../src/app/seamlylayout/qt_frontend/build/Release/SeamlyLayout.app \
        -volname "Seamly2D" $$seamlysuitedmg.target

    macSign {
        seamlysuitedmg.commands += && codesign --options runtime --timestamp -s $${APPLE_SIGN_IDENTITY} $$seamlysuitedmg.target
        seamlysuitedmg.commands += && xcrun notarytool submit --key $${APPLE_NOTARIZE_KEY} --key-id $${APPLE_NOTARIZE_KEY_ID} --issuer $${APPLE_NOTARIZE_ISSUER_ID} --wait $$seamlysuitedmg.target
        seamlysuitedmg.commands += && xcrun stapler staple -v $$seamlysuitedmg.target
    }

    TARGET = Seamly2D-macos.dmg
    first.depends = seamlysuitedmg

    QMAKE_EXTRA_TARGETS += seamlysuitedmg first
    QMAKE_CLEAN += Seamly2D-macos.dmg

    message("+++++ Exiting macos.pro +++++")
}
