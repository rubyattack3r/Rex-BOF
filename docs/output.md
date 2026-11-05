# Transcript output

Each invocation writes space-delimited transcript records from the BOF's typed
result and recovery structures. The output is not JSON.

A successful execution prints one result line. When the technique returns a
remote process identifier, the line includes it:

<!-- markdownlint-disable MD013 -->

```text
technique=wmi-exec target=10.0.0.15 status=succeeded disposition=dispatched source=win32_process_create pid=4280
```

<!-- markdownlint-enable MD013 -->

A successful check-only invocation reports `validated`:

```text
technique=wmi-exec target=10.0.0.15 status=succeeded disposition=validated source=wmi_cimv2_session
```

## Result fields

| Field | Description |
| --- | --- |
| `technique` | Canonical lowercase kebab-case public technique name. |
| `target` | Hostname or address supplied to the invocation. |
| `status` | `succeeded` or `failed`. |
| `disposition` | What the result proves: `validated`, `dispatched`, or `completed`. |
| `source` | Technique-specific evidence source. |
| `pid` | Remote process identifier when the API provides one. |
| `phase` | Failure stage: `input`, `identity`, `preflight`, `execution`, or `identity_restore`. |
| `hresult` | Native failure code on failed results. |

- `validated`: preflight passed and execution was skipped.
- `dispatched`: the remote API accepted the work. This does not show that the
  payload ran; confirm the effect on the target.
- `completed`: a synchronous command returned exit status 0.

## Failures and partial mutation

Input and preflight failures occur before the technique changes remote state.
Execution failures may include a recovery record when a change already occurred:

<!-- markdownlint-disable MD013 -->

```text
technique=scm-change-service target=10.0.0.15 status=failed phase=execution hresult=0x80070005
recovery target=10.0.0.15 action=restore_service service=Example original_binary=C:\Windows\System32\example.exe applied_binary=C:\ProgramData\staged-service.exe original_start_type=3 original_state=stopped
```

<!-- markdownlint-enable MD013 -->

Provider recovery records identify the selected role so role registration can
be removed before the shared provider and COM registration:

<!-- markdownlint-disable MD013 -->

```text
recovery target=server.example action=remove_wmi_provider provider=ExampleProvider role=instance clsid={01234567-89AB-CDEF-0123-456789ABCDEF} dll=C:\ProgramData\example-provider.dll
```

<!-- markdownlint-enable MD013 -->

## Transcript encoding

Dynamic values use UTF-8 percent encoding for whitespace, control bytes, `%`,
and `=`. A literal single `-` is `%2D`; bare `-` means unavailable.

Split a line into fields before decoding percent sequences. Path separators,
drive separators, and non-ASCII UTF-8 remain readable when they do not conflict
with the grammar. For example, `C:\Program Files\rex.dll` is emitted as
`C:\Program%20Files\rex.dll`.

Transcripts omit raw commands, launcher arguments, principals, and passwords.
See the [cleanup guide](recovery.md) for recovery fields.
