// Tests for AttachmentRepository static helpers:
// - extractImageRefs: parses grim://attachment/<uuid> from markdown
// - isGrimAttachmentUrl / attachmentIdFromUrl: URL handling
// - listAttachmentsForNote / deleteAttachment: requires DB (integration)

#include "storage/AttachmentRepository.h"

#include <QCoreApplication>
#include <QPair>
#include <QString>

namespace {

int fail(const char* what) {
    qWarning("test_attachment_refs FAILED: %s", what);
    return 1;
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    // --- 1. extractImageRefs: single reference ---
    {
        const QString md =
            "Some text\n![image](grim://attachment/1b7b27ee-be5e-4717-8cbb-9d8e38fc33fe)\nMore text";
        const auto refs = AttachmentRepository::extractImageRefs(md);
        if (refs.size() != 1) {
            return fail("expected 1 ref from single-image markdown");
        }
        if (refs[0].second != "1b7b27ee-be5e-4717-8cbb-9d8e38fc33fe") {
            return fail("wrong UUID extracted from single-image markdown");
        }
    }

    // --- 2. extractImageRefs: multiple references ---
    {
        const QString md =
            "![a](grim://attachment/aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa)\n"
            "text between\n"
            "![b](grim://attachment/bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb)\n";
        const auto refs = AttachmentRepository::extractImageRefs(md);
        if (refs.size() != 2) {
            return fail("expected 2 refs from multi-image markdown");
        }
        if (refs[0].second != "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa") {
            return fail("wrong first UUID in multi-image");
        }
        if (refs[1].second != "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb") {
            return fail("wrong second UUID in multi-image");
        }
    }

    // --- 3. extractImageRefs: no references ---
    {
        const auto refs = AttachmentRepository::extractImageRefs("Just plain text, no images.");
        if (!refs.isEmpty()) {
            return fail("expected 0 refs from plain text");
        }
    }

    // --- 4. extractImageRefs: non-grim image URLs are ignored ---
    {
        const QString md = "![photo](https://example.com/image.png)\n";
        const auto refs = AttachmentRepository::extractImageRefs(md);
        if (!refs.isEmpty()) {
            return fail("expected 0 refs for non-grim image URL");
        }
    }

    // --- 5. extractImageRefs: malformed markdown (missing alt bracket) ---
    {
        const QString md = "![broken(grim://attachment/cccccccc-cccc-cccc-cccc-cccccccccccc)\n";
        const auto refs = AttachmentRepository::extractImageRefs(md);
        if (!refs.isEmpty()) {
            return fail("expected 0 refs for malformed markdown");
        }
    }

    // --- 6. isGrimAttachmentUrl ---
    {
        if (!AttachmentRepository::isGrimAttachmentUrl(
                "grim://attachment/1b7b27ee-be5e-4717-8cbb-9d8e38fc33fe")) {
            return fail("isGrimAttachmentUrl returned false for valid URL");
        }
        if (AttachmentRepository::isGrimAttachmentUrl("https://example.com/image.png")) {
            return fail("isGrimAttachmentUrl returned true for non-grim URL");
        }
        if (AttachmentRepository::isGrimAttachmentUrl("")) {
            return fail("isGrimAttachmentUrl returned true for empty string");
        }
    }

    // --- 7. attachmentIdFromUrl ---
    {
        const QString id = AttachmentRepository::attachmentIdFromUrl(
            "grim://attachment/1b7b27ee-be5e-4717-8cbb-9d8e38fc33fe");
        if (id != "1b7b27ee-be5e-4717-8cbb-9d8e38fc33fe") {
            return fail("attachmentIdFromUrl wrong result for valid URL");
        }
        if (!AttachmentRepository::attachmentIdFromUrl("not-a-grim-url").isEmpty()) {
            return fail("attachmentIdFromUrl should return empty for non-grim URL");
        }
    }

    // --- 8. extractImageRefs: URL with full grim scheme in first captured group ---
    {
        const QString md = "![img](grim://attachment/DEADBEEF-dead-beef-dead-beefdeadbeef)\n";
        const auto refs = AttachmentRepository::extractImageRefs(md);
        if (refs.size() != 1) {
            return fail("expected 1 ref for uppercase hex UUID");
        }
        if (refs[0].first != "grim://attachment/DEADBEEF-dead-beef-dead-beefdeadbeef") {
            return fail("first element should be full grim:// URL");
        }
    }

    return 0;
}
