/****************************************************************************
** Meta object code from reading C++ file 'GrimVaultController.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../mobile/GrimVaultController.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'GrimVaultController.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN19GrimVaultControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto GrimVaultController::qt_create_metaobjectdata<qt_meta_tag_ZN19GrimVaultControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "GrimVaultController",
        "unlockedChanged",
        "",
        "vaultExistsChanged",
        "accentColorChanged",
        "errorOccurred",
        "message",
        "unlock",
        "password",
        "createVault",
        "lock",
        "biometricUnlock",
        "biometricSupported",
        "biometricConfigured",
        "enableBiometric",
        "disableBiometric",
        "noteSummaries",
        "QVariantList",
        "noteBody",
        "noteId",
        "saveNote",
        "title",
        "body",
        "createNote",
        "credentialSummaries",
        "credentialPassword",
        "id",
        "saveSettings",
        "lineNumbers",
        "wordWrap",
        "autoLock",
        "autoLockMin",
        "resetSettings",
        "unlocked",
        "vaultExists",
        "accentColor"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'unlockedChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'vaultExistsChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'accentColorChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 6 },
        }}),
        // Method 'unlock'
        QtMocHelpers::MethodData<bool(const QString &)>(7, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 8 },
        }}),
        // Method 'createVault'
        QtMocHelpers::MethodData<bool(const QString &)>(9, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 8 },
        }}),
        // Method 'lock'
        QtMocHelpers::MethodData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'biometricUnlock'
        QtMocHelpers::MethodData<bool()>(11, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'biometricSupported'
        QtMocHelpers::MethodData<bool() const>(12, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'biometricConfigured'
        QtMocHelpers::MethodData<bool() const>(13, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'enableBiometric'
        QtMocHelpers::MethodData<bool(const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 8 },
        }}),
        // Method 'disableBiometric'
        QtMocHelpers::MethodData<void()>(15, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'noteSummaries'
        QtMocHelpers::MethodData<QVariantList() const>(16, 2, QMC::AccessPublic, 0x80000000 | 17),
        // Method 'noteBody'
        QtMocHelpers::MethodData<QString(qint64) const>(18, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::LongLong, 19 },
        }}),
        // Method 'saveNote'
        QtMocHelpers::MethodData<bool(qint64, const QString &, const QString &)>(20, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::LongLong, 19 }, { QMetaType::QString, 21 }, { QMetaType::QString, 22 },
        }}),
        // Method 'createNote'
        QtMocHelpers::MethodData<qint64(const QString &)>(23, 2, QMC::AccessPublic, QMetaType::LongLong, {{
            { QMetaType::QString, 21 },
        }}),
        // Method 'credentialSummaries'
        QtMocHelpers::MethodData<QVariantList() const>(24, 2, QMC::AccessPublic, 0x80000000 | 17),
        // Method 'credentialPassword'
        QtMocHelpers::MethodData<QString(qint64) const>(25, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::LongLong, 26 },
        }}),
        // Method 'saveSettings'
        QtMocHelpers::MethodData<void(bool, bool, bool, int)>(27, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 28 }, { QMetaType::Bool, 29 }, { QMetaType::Bool, 30 }, { QMetaType::Int, 31 },
        }}),
        // Method 'resetSettings'
        QtMocHelpers::MethodData<void()>(32, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'lineNumbers'
        QtMocHelpers::MethodData<bool() const>(28, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'wordWrap'
        QtMocHelpers::MethodData<bool() const>(29, 2, QMC::AccessPublic, QMetaType::Bool),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'unlocked'
        QtMocHelpers::PropertyData<bool>(33, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'vaultExists'
        QtMocHelpers::PropertyData<bool>(34, QMetaType::Bool, QMC::DefaultPropertyFlags, 1),
        // property 'accentColor'
        QtMocHelpers::PropertyData<QString>(35, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 2),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<GrimVaultController, qt_meta_tag_ZN19GrimVaultControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject GrimVaultController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19GrimVaultControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19GrimVaultControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN19GrimVaultControllerE_t>.metaTypes,
    nullptr
} };

void GrimVaultController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<GrimVaultController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->unlockedChanged(); break;
        case 1: _t->vaultExistsChanged(); break;
        case 2: _t->accentColorChanged(); break;
        case 3: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: { bool _r = _t->unlock((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 5: { bool _r = _t->createVault((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 6: _t->lock(); break;
        case 7: { bool _r = _t->biometricUnlock();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 8: { bool _r = _t->biometricSupported();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 9: { bool _r = _t->biometricConfigured();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 10: { bool _r = _t->enableBiometric((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 11: _t->disableBiometric(); break;
        case 12: { QVariantList _r = _t->noteSummaries();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 13: { QString _r = _t->noteBody((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 14: { bool _r = _t->saveNote((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 15: { qint64 _r = _t->createNote((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<qint64*>(_a[0]) = std::move(_r); }  break;
        case 16: { QVariantList _r = _t->credentialSummaries();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 17: { QString _r = _t->credentialPassword((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 18: _t->saveSettings((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[4]))); break;
        case 19: _t->resetSettings(); break;
        case 20: { bool _r = _t->lineNumbers();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 21: { bool _r = _t->wordWrap();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (GrimVaultController::*)()>(_a, &GrimVaultController::unlockedChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (GrimVaultController::*)()>(_a, &GrimVaultController::vaultExistsChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (GrimVaultController::*)()>(_a, &GrimVaultController::accentColorChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (GrimVaultController::*)(const QString & )>(_a, &GrimVaultController::errorOccurred, 3))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->isUnlocked(); break;
        case 1: *reinterpret_cast<bool*>(_v) = _t->vaultExists(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->accentColor(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 2: _t->setAccentColor(*reinterpret_cast<QString*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *GrimVaultController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GrimVaultController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19GrimVaultControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int GrimVaultController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 22)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 22;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 22)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 22;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void GrimVaultController::unlockedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void GrimVaultController::vaultExistsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void GrimVaultController::accentColorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void GrimVaultController::errorOccurred(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}
QT_WARNING_POP
