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

struct ExportResult {
    int ok = 0;
    int failed = 0;
    int skippedIntegrity = 0;
    int collisions = 0;

    bool allOk() const { return failed == 0 && skippedIntegrity == 0; }
};
