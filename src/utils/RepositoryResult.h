#pragma once

#include <QString>
#include <optional>

enum class DecryptStatus {
    Ok,
    Empty,
    IntegrityError,
};

template<typename T>
struct DecryptResult {
    DecryptStatus status = DecryptStatus::IntegrityError;
    T value{};

    bool isOk() const { return status == DecryptStatus::Ok || status == DecryptStatus::Empty; }
    bool hasIntegrityError() const { return status == DecryptStatus::IntegrityError; }
};

struct CredentialLoadResult {
    bool ok = false;
    bool integrityError = false;
    QString errorMessage;
};
