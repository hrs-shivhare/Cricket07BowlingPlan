# Diagnostic executable hash — version 2.5

The user confirmed version 2.4 gameplay works. Version 2.5 changes only executable compatibility diagnostics and initialization organization. Every bowling, field-family, threshold, circle and event-triggered variant rule remains unchanged.

Runtime behavior:
- Calculate the running executable's SHA-256 and log the actual digest.
- A reference match logs success. A mismatch logs WARNING and continues. An unreadable/unhashable file also warns and continues.
- x86 PE identity, the existing expected image size and all nine hook instruction prefixes still gate installation. Layout failure produces status -1; hook/signature failure produces -2; successful installation is 1.
- InitializeASI remains idempotent. Full-file hashing never decides whether hooks are allowed.

Build behavior:
- A reference-hash mismatch warns instead of stopping.
- Existing executable PE shape and hook-prefix tests still run.
- Before/after hashes are compared to detect an unexpected file change during validation.

Validation passed:
- Hash the actual game executable; copy it to a temporary file and append a 24-byte footer. The copied file has a different SHA-256 while retaining the expected PE layout and all nine native hook prefixes. The temporary copy is deleted; the installed EXE is never modified.
- Hashing the deleted file reports unavailable separately from mismatch.
- Run the production initializer against a compatible synthetic mapped image while the current test-host executable has a nonmatching SHA-256. All nine hooks install and status becomes 1.
- The same nonmatching host still rejects an AMD64 PE layout with status -1 and a modified hook prefix with status -2, without installing hooks.
- Existing DLL idempotence/status, all field-variant and field-family rules, circle enforcement, 31,500 seeded schedules, 521 resume cases, detour/trampoline and actual-EXE-prefix regressions pass.

This is not proof that arbitrary modified Cricket07 binaries are compatible: changed uninspected helper code or structure offsets can still matter. No foreign executable build was tested live.

ASI SHA-256: c5204ac280948f3841a9402678e75963333832bfc6650db3bfcd984ae4f8ab8c.
Reference EXE SHA-256: d776dba0cc506b968f7d228e036cf42aeb4abe45a8d1ad0d7226cdebf91553a2.
The 2.4 package remains preserved in output/Cricket07BowlingPlan-2.4.zip.