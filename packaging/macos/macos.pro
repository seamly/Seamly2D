message(" ")
message("===== Entering macos.pro =====")
message(" ")

TEMPLATE = aux

macx{
    APPLE_SIGN_IDENTITY = $$shell_quote($(APPLE_SIGN_IDENTITY))
    APPLE_NOTARIZE_KEY = $(APPLE_NOTARIZE_KEY)
    APPLE_NOTARIZE_KEY_ID = $(APPLE_NOTARIZE_KEY_ID)
    APPLE_NOTARIZE_ISSUER_ID = $(APPLE_NOTARIZE_ISSUER_ID)

    seamly2ddmg.target = Seamly2D.dmg
    seamly2ddmg.commands = mkdir -p $${OUT_PWD}
    seamly2ddmg.commands += && hdiutil create -srcfolder $${OUT_PWD}/../src/app/seamly2d/bin/Seamly2D.app -volname "Seamly2D" $$seamly2ddmg.target

    macSign {
        seamly2ddmg.commands += && codesign --options runtime --timestamp -s $${APPLE_SIGN_IDENTITY} $$seamly2ddmg.target
        seamly2ddmg.commands += && xcrun notarytool submit --key $${APPLE_NOTARIZE_KEY} --key-id $${APPLE_NOTARIZE_KEY_ID} --issuer $${APPLE_NOTARIZE_ISSUER_ID} --wait $$seamly2ddmg.target
        seamly2ddmg.commands += && xcrun stapler staple -v $$seamly2ddmg.target
    }

    seamlymedmg.target = SeamlyME.dmg
    seamlymedmg.depends = seamly2ddmg  # updated by slspencer on 20260527 - serialize hdiutil to avoid "Resource busy" under make -j
    seamlymedmg.commands = mkdir -p $${OUT_PWD}
    seamlymedmg.commands += && hdiutil create -fs HFS+ -srcfolder $${OUT_PWD}/../src/app/seamlyme/bin/seamlyme.app -volname "SeamlyME" $$seamlymedmg.target

    macSign {
        seamlymedmg.commands += && codesign --options runtime --timestamp -s $${APPLE_SIGN_IDENTITY} $$seamlymedmg.target
        seamlymedmg.commands += && xcrun notarytool submit --key $${APPLE_NOTARIZE_KEY} --key-id $${APPLE_NOTARIZE_KEY_ID} --issuer $${APPLE_NOTARIZE_ISSUER_ID} --wait $$seamlymedmg.target
        seamlymedmg.commands += && xcrun stapler staple -v $$seamlymedmg.target
    }

    # SeamlyLayout.app is built by CMake/Cargo before qmake runs (see ci.yml's
    # 'Build SeamlyLayout release' step) — this rule only packages the bundle
    # CMake already produced, the same way seamly2ddmg/seamlymedmg package the
    # qmake-built .app bundles.
    seamlylayoutdmg.target = SeamlyLayout.dmg
    seamlylayoutdmg.depends = seamlymedmg  # serialize hdiutil to avoid "Resource busy" under make -j
    seamlylayoutdmg.commands = mkdir -p $${OUT_PWD}
    seamlylayoutdmg.commands += && hdiutil create -srcfolder $${OUT_PWD}/../src/app/seamlylayout/qt_frontend/build/Release/SeamlyLayout.app -volname "SeamlyLayout" $$seamlylayoutdmg.target

    macSign {
        seamlylayoutdmg.commands += && codesign --options runtime --timestamp -s $${APPLE_SIGN_IDENTITY} $$seamlylayoutdmg.target
        seamlylayoutdmg.commands += && xcrun notarytool submit --key $${APPLE_NOTARIZE_KEY} --key-id $${APPLE_NOTARIZE_KEY_ID} --issuer $${APPLE_NOTARIZE_ISSUER_ID} --wait $$seamlylayoutdmg.target
        seamlylayoutdmg.commands += && xcrun stapler staple -v $$seamlylayoutdmg.target
    }

    TARGET = Seamly2D-macos.zip
    first.commands = zip Seamly2D-macos.zip Seamly2D.dmg SeamlyME.dmg SeamlyLayout.dmg
    first.depends = seamly2ddmg seamlymedmg seamlylayoutdmg

    QMAKE_EXTRA_TARGETS += seamly2ddmg seamlymedmg seamlylayoutdmg first
    QMAKE_CLEAN += Seamly2D.dmg SeamlyME.dmg SeamlyLayout.dmg Seamly2D-macos.zip

    message("+++++ Exiting macos.pro +++++")
}
