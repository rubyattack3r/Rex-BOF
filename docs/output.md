# Transcript output

Each invocation emits concise transcript records backed by typed C result, observation, and recovery structs. The BOF output is not a versioned JSON schema and does not mirror the Go CLI serialization contract.

Successful execution emits one result line. Techniques that expose a remote process identifier include it:

```text
technique=WMIExec target=10.0.0.15 status=succeeded disposition=dispatched source=win32_process_create pid=4280
```

A successful check-only invocation reports `validated`:

```text
technique=WMIExec target=10.0.0.15 status=succeeded disposition=validated source=wmi_cimv2_session
```

## Result fields

| Field | Description |
|---|---|
| `technique` | Stable public selector. |
| `target` | Hostname or address supplied to the invocation. |
| `status` | `succeeded` or `failed`. |
| `disposition` | What the remote API proved: `validated`, `dispatched`, or `completed`. |
| `source` | Technique-specific evidence source. |
| `pid` | Remote process identifier when the API provides one. |
| `phase` | Failure boundary: `input`, `preflight`, or `execution`. |
| `hresult` | Native failure code on failed results. |

`validated` means the registered read-only prerequisite callback succeeded and execution was skipped. `dispatched` means the remote API accepted or exposed the work but did not prove payload completion. `completed` means a synchronous API reported a zero exit status.

## Failures and partial mutation

Input and preflight failures occur before a technique-owned persistent mutation. Execution can fail after a mutation; when that happens, the BOF emits the failure and the available recovery record:

```text
technique=SCMChangeService target=10.0.0.15 status=failed phase=execution hresult=0x80070005
recovery target=10.0.0.15 action=restore_service service=Example original_binary=C:\Windows\System32\example.exe applied_binary=C:\ProgramData\staged-service.exe original_start_type=3 original_state=stopped
```

## Transcript encoding

Records are space-delimited `key=value` fields. Dynamic recovery values use reversible UTF-8 percent encoding when whitespace, control bytes, `%`, or `=` would make the record ambiguous. A literal single `-` is `%2D`; bare `-` means the value was unavailable.

Split a line into fields before decoding percent sequences. Windows path separators, drive separators, and non-ASCII UTF-8 remain readable when they do not conflict with the grammar. For example, `C:\Program Files\rex.dll` is emitted as `C:\Program%20Files\rex.dll`.

Raw commands, launcher arguments, and credentials are never copied into the transcript. See [operator recovery](recovery.md) for the meaning of recovery records.

