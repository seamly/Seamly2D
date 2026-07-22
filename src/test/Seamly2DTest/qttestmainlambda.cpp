//-----------------------------------------------------------------------------
//  @file   qttestmainlambda.cpp
//  @author Douglas S Caskey
//  @date   13 July, 2025
//
//  @brief
//  @copyright
//  This source code is part of the Seamly2D project, a pattern making
//  program to create and model patterns of clothing.
//  Copyright (C) 2017-2025 Seamly2D project
//  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
//
//  Seamly2D is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Seamly2D is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Seamly2D.  If not, see <http://www.gnu.org/licenses/>.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  @file   qttestmainlambda.cpp
//  @author Roman Telezhynskyi <dismine(at)gmail.com>
//  @date    31 3, 2015
//
//  @brief
//  @copyright
//  This source code is part of the Valentina project, a pattern making
//  program, whose allow create and modeling patterns of clothing.
//  Copyright (C) 2015 Valentina project
//  <https://bitbucket.org/dismine/valentina> All Rights Reserved.
//
//  Valentina is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Valentina is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
//-----------------------------------------------------------------------------

#include <QDir>
#include <QtTest>

#include "tst_vposter.h"
#include "tst_vabstractpiece.h"
#include "tst_vspline.h"
#include "tst_nameregexp.h"
#include "tst_vlayoutdetail.h"
#include "tst_varc.h"
#include "tst_vellipticalarc.h"
#include "tst_qmutokenparser.h"
#include "tst_vmeasurements.h"
#include "tst_vlockguard.h"
#include "tst_misc.h"
#include "tst_vcommandline.h"
#include "tst_vpiece.h"
#include "tst_findpoint.h"
#include "tst_vabstractcurve.h"
#include "tst_vcubicbezierpath.h"
#include "tst_vgobject.h"
#include "tst_vsplinepath.h"
#include "tst_vpointf.h"
#include "tst_readval.h"
#include "tst_vtranslatevars.h"
#include "tst_svgtextitem.h"
#include "tst_svgcomponenttags.h"
#include "tst_seamlyfamilypaths.h"

#include "../vmisc/def.h"
#include "../qmuparser/qmudef.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/projectversion.h"

class TestApplication2D : public VAbstractApplication
{
public:

                                  TestApplication2D(int &argc, char ** argv);
    virtual                      ~TestApplication2D() Q_DECL_EQ_DEFAULT;

    virtual const VTranslateVars *translateVariables();
    virtual void                  openSettings();
    virtual bool                  isAppInGUIMode() const;
    virtual void                  initTranslateVariables();
};

//---------------------------------------------------------------------------------------------------------------------
TestApplication2D::TestApplication2D(int &argc, char **argv)
    : VAbstractApplication(argc, argv)
{
    setApplicationName(VER_INTERNALNAME_2D_STR);
    setOrganizationName(VER_COMPANYNAME_STR);
    openSettings();
}

//---------------------------------------------------------------------------------------------------------------------
const VTranslateVars *TestApplication2D::translateVariables()
{
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
// Task 15: mirrors Application2D::openSettings() so the test suite resolves settings the
// same way the real app does. No migration notice is ever shown here — tests must never
// block on a modal dialog — so MigrateSeamlySettingsLocation() is called with a null
// out-parameter, which the shared helper treats as "caller doesn't need to know".
void TestApplication2D::openSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());

    const QString dir = QFileInfo(settings.fileName()).absolutePath();
    const QString qt5Common   = dir + "/common.ini";
    const QString qt6Common   = dir + "/qt6_common.ini";

    // QFile::copy() never creates missing parent directories, and the "Seamly" organization
    // folder does not exist yet the very first time any app runs under the renamed
    // organization.
    QDir().mkpath(dir);

    static const QString kLegacyOrganizationName = QStringLiteral("Seamly2DTeam");
    const QSettings legacyCommonProbe(QSettings::IniFormat, QSettings::UserScope,
                                      kLegacyOrganizationName, QCoreApplication::applicationName());
    const QString legacyDir = QFileInfo(legacyCommonProbe.fileName()).absolutePath();
    if (!QFileInfo::exists(qt6Common) && QFileInfo::exists(legacyDir + "/qt6_common.ini"))
    {
        QFile::copy(legacyDir + "/qt6_common.ini", qt6Common);
    }
    else if (!QFileInfo::exists(qt5Common) && QFileInfo::exists(legacyDir + "/common.ini"))
    {
        QFile::copy(legacyDir + "/common.ini", qt5Common);
    }

    if (!QFileInfo::exists(qt6Common) && QFileInfo::exists(qt5Common))
    {
        QFile::copy(qt5Common, qt6Common);
    }

    const QString qt6Settings = MigrateSeamlySettingsLocation(
        QStringLiteral("qt6_seamly2d.ini"),
        { QStringLiteral("qt6_seamly2d.ini"), QStringLiteral("Seamly2D.ini") });

    m_settings = new VSettings(qt6Settings, QSettings::IniFormat, this);
}

//---------------------------------------------------------------------------------------------------------------------
bool TestApplication2D::isAppInGUIMode() const
{
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
void TestApplication2D::initTranslateVariables()
{
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Run every registered test suite in one process and OR their exit codes.
 *
 * Each suite is executed through QTest::qExec() by the ASSERT_TEST lambda; the
 * combined status (0 only when every suite passed) is the process exit code.
 *
 * Per-suite log capture: when the environment variable SEAMLY_TEST_LOG_DIR is
 * set to a directory, each suite additionally writes a plain-text QTest log to
 * "<dir>/<SuiteClassName>.txt" via an injected "-o file,txt" argument. Without
 * it, a single "-o" on the command line is overwritten by every subsequent
 * qExec() call, and on Windows the console/stdout output of the suite can be
 * lost entirely when redirected — so this hook is what makes local per-suite
 * results capturable at all (used by scripts/st.ps1; see Task 23).
 *
 * @param argc argument count, forwarded to every QTest::qExec() call
 * @param argv argument values, forwarded to every QTest::qExec() call
 * @return OR-ed QTest failure status across all suites (0 = all passed)
 */
int main(int argc, char** argv)
{
    Q_INIT_RESOURCE(schema);

    TestApplication2D app( argc, argv );// For QPrinter

    // Optional directory for per-suite plain-text QTest logs (empty = disabled).
    const QString logDir = qEnvironmentVariable("SEAMLY_TEST_LOG_DIR");

    int status = 0;
    auto ASSERT_TEST = [&status, &logDir, argc, argv](QObject* obj)
    {
        if (logDir.isEmpty())
        {
            // Default behavior: forward the process arguments unchanged.
            status |= QTest::qExec(obj, argc, argv);
        }
        else
        {
            // Rebuild the argument list and append a per-suite file logger so
            // each suite's output survives in its own file instead of every
            // qExec() overwriting one shared "-o" target.
            QStringList args;
            args.reserve(argc + 2);
            for (int i = 0; i < argc; ++i)
            {
                args << QString::fromLocal8Bit(argv[i]);
            }
            const QString suiteName = QString::fromLatin1(obj->metaObject()->className());
            args << QStringLiteral("-o")
                 << QStringLiteral("%1/%2.txt,txt").arg(logDir, suiteName);
            status |= QTest::qExec(obj, args);
        }
        delete obj;
    };

    ASSERT_TEST(new TST_FindPoint());
    ASSERT_TEST(new TST_VPiece());
    ASSERT_TEST(new TST_VPoster());
    ASSERT_TEST(new TST_VAbstractPiece());
    ASSERT_TEST(new TST_VSpline());
    ASSERT_TEST(new TST_VSplinePath());
    ASSERT_TEST(new TST_NameRegExp());
    ASSERT_TEST(new TST_VLayoutDetail());
    ASSERT_TEST(new TST_VArc());
    ASSERT_TEST(new TST_VEllipticalArc());
    ASSERT_TEST(new TST_QmuTokenParser());
    ASSERT_TEST(new TST_Measurements());
    ASSERT_TEST(new TST_VLockGuard());
    ASSERT_TEST(new TST_Misc());
    ASSERT_TEST(new TST_VCommandLine());
    ASSERT_TEST(new TST_VAbstractCurve());
    ASSERT_TEST(new TST_VCubicBezierPath());
    ASSERT_TEST(new TST_VGObject());
    ASSERT_TEST(new TST_VPointF());
    ASSERT_TEST(new TST_ReadVal());
    ASSERT_TEST(new TST_VTranslateVars());
    ASSERT_TEST(new TST_SvgTextItem());
    ASSERT_TEST(new TST_SvgComponentTags());
    ASSERT_TEST(new TST_SeamlyFamilyPaths());

    return status;
}
