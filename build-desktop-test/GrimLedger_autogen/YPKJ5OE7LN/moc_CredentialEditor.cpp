/****************************************************************************
** Meta object code from reading C++ file 'CredentialEditor.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/CredentialEditor.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CredentialEditor.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN16CredentialEditorE_t {};
} // unnamed namespace

template <> constexpr inline auto CredentialEditor::qt_create_metaobjectdata<qt_meta_tag_ZN16CredentialEditorE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "CredentialEditor",
        "contentChanged",
        "",
        "saveRequested",
        "deleteRequested",
        "generatePasswordRequested",
        "copyPasswordRequested",
        "copyUsernameRequested",
        "copyTotpRequested",
        "checkBreachRequested"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'contentChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'saveRequested'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'deleteRequested'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'generatePasswordRequested'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'copyPasswordRequested'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'copyUsernameRequested'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'copyTotpRequested'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'checkBreachRequested'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<CredentialEditor, qt_meta_tag_ZN16CredentialEditorE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject CredentialEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16CredentialEditorE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16CredentialEditorE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN16CredentialEditorE_t>.metaTypes,
    nullptr
} };

void CredentialEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CredentialEditor *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->contentChanged(); break;
        case 1: _t->saveRequested(); break;
        case 2: _t->deleteRequested(); break;
        case 3: _t->generatePasswordRequested(); break;
        case 4: _t->copyPasswordRequested(); break;
        case 5: _t->copyUsernameRequested(); break;
        case 6: _t->copyTotpRequested(); break;
        case 7: _t->checkBreachRequested(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (CredentialEditor::*)()>(_a, &CredentialEditor::contentChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (CredentialEditor::*)()>(_a, &CredentialEditor::saveRequested, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (CredentialEditor::*)()>(_a, &CredentialEditor::deleteRequested, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (CredentialEditor::*)()>(_a, &CredentialEditor::generatePasswordRequested, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (CredentialEditor::*)()>(_a, &CredentialEditor::copyPasswordRequested, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (CredentialEditor::*)()>(_a, &CredentialEditor::copyUsernameRequested, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (CredentialEditor::*)()>(_a, &CredentialEditor::copyTotpRequested, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (CredentialEditor::*)()>(_a, &CredentialEditor::checkBreachRequested, 7))
            return;
    }
}

const QMetaObject *CredentialEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CredentialEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16CredentialEditorE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int CredentialEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void CredentialEditor::contentChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void CredentialEditor::saveRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void CredentialEditor::deleteRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void CredentialEditor::generatePasswordRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void CredentialEditor::copyPasswordRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void CredentialEditor::copyUsernameRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void CredentialEditor::copyTotpRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void CredentialEditor::checkBreachRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}
QT_WARNING_POP
