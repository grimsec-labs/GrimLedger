/****************************************************************************
** Meta object code from reading C++ file 'SettingsWindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/SettingsWindow.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SettingsWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN14SettingsWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto SettingsWindow::qt_create_metaobjectdata<qt_meta_tag_ZN14SettingsWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SettingsWindow",
        "accentPreviewChanged",
        "",
        "hex",
        "accentChanged",
        "lineNumbersChanged",
        "enabled",
        "wordWrapChanged",
        "autoLockChanged",
        "minutes",
        "browserBridgeChanged",
        "hibpCheckChanged",
        "enableHelloUnlockRequested",
        "disableHelloUnlockRequested",
        "changePasswordRequested",
        "current",
        "newPass",
        "backupVaultRequested",
        "path",
        "restoreVaultRequested",
        "importMarkdownRequested",
        "importCredentialsRequested",
        "exportNoteRequested",
        "exportAllMarkdownRequested",
        "exportEncryptedArchiveRequested",
        "refreshHealthRequested",
        "scanSecretsRequested",
        "redactExportRequested",
        "startRunbookRequested",
        "showKnowledgeGraphRequested",
        "showChronicleRequested",
        "exportGrimShareRequested",
        "importGrimShareRequested",
        "lockWorkChamberRequested",
        "unlockWorkChamberRequested",
        "webClipperChanged",
        "semanticSearchChanged",
        "backRequested"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'accentPreviewChanged'
        QtMocHelpers::SignalData<void(const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'accentChanged'
        QtMocHelpers::SignalData<void(const QString &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'lineNumbersChanged'
        QtMocHelpers::SignalData<void(bool)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 6 },
        }}),
        // Signal 'wordWrapChanged'
        QtMocHelpers::SignalData<void(bool)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 6 },
        }}),
        // Signal 'autoLockChanged'
        QtMocHelpers::SignalData<void(bool, int)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 6 }, { QMetaType::Int, 9 },
        }}),
        // Signal 'browserBridgeChanged'
        QtMocHelpers::SignalData<void(bool)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 6 },
        }}),
        // Signal 'hibpCheckChanged'
        QtMocHelpers::SignalData<void(bool)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 6 },
        }}),
        // Signal 'enableHelloUnlockRequested'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'disableHelloUnlockRequested'
        QtMocHelpers::SignalData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'changePasswordRequested'
        QtMocHelpers::SignalData<void(const QString &, const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 15 }, { QMetaType::QString, 16 },
        }}),
        // Signal 'backupVaultRequested'
        QtMocHelpers::SignalData<void(const QString &)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 18 },
        }}),
        // Signal 'restoreVaultRequested'
        QtMocHelpers::SignalData<void(const QString &)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 18 },
        }}),
        // Signal 'importMarkdownRequested'
        QtMocHelpers::SignalData<void()>(20, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'importCredentialsRequested'
        QtMocHelpers::SignalData<void()>(21, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'exportNoteRequested'
        QtMocHelpers::SignalData<void()>(22, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'exportAllMarkdownRequested'
        QtMocHelpers::SignalData<void()>(23, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'exportEncryptedArchiveRequested'
        QtMocHelpers::SignalData<void(const QString &)>(24, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 18 },
        }}),
        // Signal 'refreshHealthRequested'
        QtMocHelpers::SignalData<void()>(25, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'scanSecretsRequested'
        QtMocHelpers::SignalData<void()>(26, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'redactExportRequested'
        QtMocHelpers::SignalData<void()>(27, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'startRunbookRequested'
        QtMocHelpers::SignalData<void()>(28, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'showKnowledgeGraphRequested'
        QtMocHelpers::SignalData<void()>(29, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'showChronicleRequested'
        QtMocHelpers::SignalData<void()>(30, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'exportGrimShareRequested'
        QtMocHelpers::SignalData<void()>(31, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'importGrimShareRequested'
        QtMocHelpers::SignalData<void()>(32, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'lockWorkChamberRequested'
        QtMocHelpers::SignalData<void()>(33, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'unlockWorkChamberRequested'
        QtMocHelpers::SignalData<void()>(34, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'webClipperChanged'
        QtMocHelpers::SignalData<void(bool)>(35, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 6 },
        }}),
        // Signal 'semanticSearchChanged'
        QtMocHelpers::SignalData<void(bool)>(36, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 6 },
        }}),
        // Signal 'backRequested'
        QtMocHelpers::SignalData<void()>(37, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<SettingsWindow, qt_meta_tag_ZN14SettingsWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SettingsWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14SettingsWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14SettingsWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14SettingsWindowE_t>.metaTypes,
    nullptr
} };

void SettingsWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SettingsWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->accentPreviewChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->accentChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->lineNumbersChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->wordWrapChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 4: _t->autoLockChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 5: _t->browserBridgeChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 6: _t->hibpCheckChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 7: _t->enableHelloUnlockRequested(); break;
        case 8: _t->disableHelloUnlockRequested(); break;
        case 9: _t->changePasswordRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 10: _t->backupVaultRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->restoreVaultRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 12: _t->importMarkdownRequested(); break;
        case 13: _t->importCredentialsRequested(); break;
        case 14: _t->exportNoteRequested(); break;
        case 15: _t->exportAllMarkdownRequested(); break;
        case 16: _t->exportEncryptedArchiveRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 17: _t->refreshHealthRequested(); break;
        case 18: _t->scanSecretsRequested(); break;
        case 19: _t->redactExportRequested(); break;
        case 20: _t->startRunbookRequested(); break;
        case 21: _t->showKnowledgeGraphRequested(); break;
        case 22: _t->showChronicleRequested(); break;
        case 23: _t->exportGrimShareRequested(); break;
        case 24: _t->importGrimShareRequested(); break;
        case 25: _t->lockWorkChamberRequested(); break;
        case 26: _t->unlockWorkChamberRequested(); break;
        case 27: _t->webClipperChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 28: _t->semanticSearchChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 29: _t->backRequested(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)(const QString & )>(_a, &SettingsWindow::accentPreviewChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)(const QString & )>(_a, &SettingsWindow::accentChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)(bool )>(_a, &SettingsWindow::lineNumbersChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)(bool )>(_a, &SettingsWindow::wordWrapChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)(bool , int )>(_a, &SettingsWindow::autoLockChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)(bool )>(_a, &SettingsWindow::browserBridgeChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)(bool )>(_a, &SettingsWindow::hibpCheckChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::enableHelloUnlockRequested, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::disableHelloUnlockRequested, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)(const QString & , const QString & )>(_a, &SettingsWindow::changePasswordRequested, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)(const QString & )>(_a, &SettingsWindow::backupVaultRequested, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)(const QString & )>(_a, &SettingsWindow::restoreVaultRequested, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::importMarkdownRequested, 12))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::importCredentialsRequested, 13))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::exportNoteRequested, 14))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::exportAllMarkdownRequested, 15))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)(const QString & )>(_a, &SettingsWindow::exportEncryptedArchiveRequested, 16))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::refreshHealthRequested, 17))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::scanSecretsRequested, 18))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::redactExportRequested, 19))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::startRunbookRequested, 20))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::showKnowledgeGraphRequested, 21))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::showChronicleRequested, 22))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::exportGrimShareRequested, 23))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::importGrimShareRequested, 24))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::lockWorkChamberRequested, 25))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::unlockWorkChamberRequested, 26))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)(bool )>(_a, &SettingsWindow::webClipperChanged, 27))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)(bool )>(_a, &SettingsWindow::semanticSearchChanged, 28))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsWindow::*)()>(_a, &SettingsWindow::backRequested, 29))
            return;
    }
}

const QMetaObject *SettingsWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SettingsWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14SettingsWindowE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SettingsWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 30)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 30;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 30)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 30;
    }
    return _id;
}

// SIGNAL 0
void SettingsWindow::accentPreviewChanged(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void SettingsWindow::accentChanged(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void SettingsWindow::lineNumbersChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void SettingsWindow::wordWrapChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void SettingsWindow::autoLockChanged(bool _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}

// SIGNAL 5
void SettingsWindow::browserBridgeChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void SettingsWindow::hibpCheckChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void SettingsWindow::enableHelloUnlockRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void SettingsWindow::disableHelloUnlockRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void SettingsWindow::changePasswordRequested(const QString & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1, _t2);
}

// SIGNAL 10
void SettingsWindow::backupVaultRequested(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1);
}

// SIGNAL 11
void SettingsWindow::restoreVaultRequested(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1);
}

// SIGNAL 12
void SettingsWindow::importMarkdownRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void SettingsWindow::importCredentialsRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}

// SIGNAL 14
void SettingsWindow::exportNoteRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void SettingsWindow::exportAllMarkdownRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}

// SIGNAL 16
void SettingsWindow::exportEncryptedArchiveRequested(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 16, nullptr, _t1);
}

// SIGNAL 17
void SettingsWindow::refreshHealthRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 17, nullptr);
}

// SIGNAL 18
void SettingsWindow::scanSecretsRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 18, nullptr);
}

// SIGNAL 19
void SettingsWindow::redactExportRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 19, nullptr);
}

// SIGNAL 20
void SettingsWindow::startRunbookRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 20, nullptr);
}

// SIGNAL 21
void SettingsWindow::showKnowledgeGraphRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 21, nullptr);
}

// SIGNAL 22
void SettingsWindow::showChronicleRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 22, nullptr);
}

// SIGNAL 23
void SettingsWindow::exportGrimShareRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 23, nullptr);
}

// SIGNAL 24
void SettingsWindow::importGrimShareRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 24, nullptr);
}

// SIGNAL 25
void SettingsWindow::lockWorkChamberRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 25, nullptr);
}

// SIGNAL 26
void SettingsWindow::unlockWorkChamberRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 26, nullptr);
}

// SIGNAL 27
void SettingsWindow::webClipperChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 27, nullptr, _t1);
}

// SIGNAL 28
void SettingsWindow::semanticSearchChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 28, nullptr, _t1);
}

// SIGNAL 29
void SettingsWindow::backRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 29, nullptr);
}
QT_WARNING_POP
