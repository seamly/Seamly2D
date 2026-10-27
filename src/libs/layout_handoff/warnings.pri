#-------------------------------------------------
#
# @file   warnings.pri
# @author slspencer
# Copyright (C) 2026 Seamly2D Project
# SPDX-License-Identifier: GPL-3.0-or-later
#
# @brief
# Compiler warning and sanitizer flags of the layout_handoff library.
#
#-------------------------------------------------

unix {
    *g++*{
        QMAKE_CXXFLAGS += \
            # -isystem stops warnings from the generated moc and rcc headers.
            -isystem "$${OUT_PWD}/$${MOC_DIR}" \
            -isystem "$${OUT_PWD}/$${RCC_DIR}" \
            $$GCC_DEBUG_CXXFLAGS # Empty: common.pri does not define it.

        checkWarnings{ # Turn on with CONFIG+=checkWarnings.
            QMAKE_CXXFLAGS += -Werror
        }

        noAddressSanitizer{ # Turn off the address sanitizer with CONFIG+=noAddressSanitizer.
            # No sanitizer flags.
        } else {
            CONFIG(debug, debug|release){
                # Address sanitizer, debug builds only.
                QMAKE_CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
                QMAKE_CFLAGS += -fsanitize=address -fno-omit-frame-pointer
                QMAKE_LFLAGS += -fsanitize=address
            }
        }

        gccUbsan{ # Turn on the undefined behavior sanitizer with CONFIG+=gccUbsan.
            CONFIG(debug, debug|release){
                # Undefined behavior sanitizer, debug builds only.
                QMAKE_CXXFLAGS += -fsanitize=undefined
                QMAKE_CFLAGS += -fsanitize=undefined
                QMAKE_LFLAGS += -fsanitize=undefined
            }
        }
    }

    *clang*{
        QMAKE_CXXFLAGS += \
            # -isystem stops warnings from the generated moc and rcc headers.
            -isystem "$${OUT_PWD}/$${MOC_DIR}" \
            -isystem "$${OUT_PWD}/$${RCC_DIR}" \
            $$CLANG_DEBUG_CXXFLAGS # Empty: common.pri does not define it.

        checkWarnings{ # Turn on with CONFIG+=checkWarnings.
            QMAKE_CXXFLAGS += -Werror
        }

        # -isystem covers headers only. These warnings come from the generated moc and rcc sources.
        QMAKE_CXXFLAGS -= \
            -Wundefined-reinterpret-cast \
            -Wmissing-prototypes # rcc sources
    }

    *-icc-*{
        QMAKE_CXXFLAGS += \
            # -isystem stops warnings from the generated moc and rcc headers.
            -isystem "$${OUT_PWD}/$${MOC_DIR}" \
            -isystem "$${OUT_PWD}/$${RCC_DIR}" \
            $$ICC_DEBUG_CXXFLAGS # Empty: common.pri does not define it.

        checkWarnings{ # Turn on with CONFIG+=checkWarnings.
            QMAKE_CXXFLAGS += -Werror
        }
    }
} else { # Windows
    *g++*{
        QMAKE_CXXFLAGS += $$GCC_DEBUG_CXXFLAGS # Empty: common.pri does not define it.

        checkWarnings{ # Turn on with CONFIG+=checkWarnings.
            QMAKE_CXXFLAGS += -Werror
        }
    }

    *msvc*{
        QMAKE_CXXFLAGS += $$MSVC_DEBUG_CXXFLAGS # Empty: common.pri does not define it.

        checkWarnings{ # Turn on with CONFIG+=checkWarnings.
            QMAKE_CXXFLAGS += -WX
        }
    }
}
