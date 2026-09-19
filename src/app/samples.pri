# Path to pattern sample files
INSTALL_PATTERNS += \
    $${PWD}/share/samples/patterns/jacket1_52-176.sm2d \
    $${PWD}/share/samples/patterns/jacket2_40-146.sm2d \
    $${PWD}/share/samples/patterns/jacket3_40-146.sm2d \
    $${PWD}/share/samples/patterns/jacket4_40-146.sm2d \
    $${PWD}/share/samples/patterns/jacket5_30-110.sm2d \
    $${PWD}/share/samples/patterns/jacket6_30-110.sm2d \
    $${PWD}/share/samples/patterns/male_shirt.sm2d \
    $${PWD}/share/samples/patterns/trousers.sm2d

# Path to individual measurement sample files
INSTALL_INDIVIDUAL_MEASUREMENTS += \
    $${PWD}/share/samples/measurements/individual/male_chest_102cm.smis \
    $${PWD}/share/samples/measurements/individual/trousers.smis

# Path to multisize measurement sample files
INSTALL_MULTISIZE_MEASUREMENTS += \
    $${PWD}/share/samples/measurements/multisize/gost_man_ru.smms

# Path to measurement template sample files
INSTALL_STANDARD_TEMPLATES += \
    $${PWD}/share/samples/measurements/templates/all_measurements_template.smis \
    $${PWD}/share/samples/measurements/templates/aldrich_women_template.smis

# Path to label template sample files
INSTALL_LABEL_TEMPLATES += \
    $${PWD}/share/labels/default_pattern_label.xml \
    $${PWD}/share/labels/default_piece_label.xml

win32
{
    copyToDestdir($$INSTALL_PATTERNS, $$shell_path($${OUT_PWD}/$${DESTDIR}/samples/patterns))
    copyToDestdir($$INSTALL_INDIVIDUAL_MEASUREMENTS, $$shell_path($${OUT_PWD}/$${DESTDIR}/samples/measurements/individual))
    copyToDestdir($$INSTALL_MULTISIZE_MEASUREMENTS, $$shell_path($${OUT_PWD}/$${DESTDIR}/samples/measurements/multisize))
    copyToDestdir($$INSTALL_STANDARD_TEMPLATES, $$shell_path($${OUT_PWD}/$${DESTDIR}/samples/measurements/templates))
    copyToDestdir($$INSTALL_LABEL_TEMPLATES, $$shell_path($${OUT_PWD}/$${DESTDIR}/labels))
}
