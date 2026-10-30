# WMI event-consumer command contract

`wmi-event-subscription` stores the supplied `--command` unchanged as the
`CommandLineTemplate` of a permanent `CommandLineEventConsumer`. It does not
resolve a launcher or insert `cmd.exe` or PowerShell.

## Required contract

Supply one complete Windows command line containing the target-side executable
and all arguments it needs. Use absolute paths and Windows command-line quoting
for paths with spaces. The executable and its dependencies must already exist
on the target.

The standard consumer starts the process in the local-system context when a
matching event arrives. The command must not depend on the interactive user's
profile, mapped drives, current directory, or desktop. See Microsoft's
[`CommandLineEventConsumer`](https://learn.microsoft.com/en-us/windows/win32/wmisdk/commandlineeventconsumer)
reference.

The WQL query is evaluated in `root\cimv2`. Confirm that its event class,
properties, polling interval, and expected frequency are valid for the target.
Every matching event can start another process until the binding is removed.

## Validation

Before execution:

1. Run the exact command under the intended noninteractive target context.
2. Test the WQL query separately and confirm its expected event frequency.
3. Confirm that repeated events cannot create an unsafe number of concurrent
   processes.
4. Define cleanup for the command's files, processes, and other side effects
   in addition to removing the WMI subscription.

Rex preflight checks only that the filter and consumer names are unused. It
does not parse the WQL query, resolve the executable, validate quoting, or run
the command. A successful Rex execution means the filter, consumer, and
binding were created; it does not mean an event fired or the command succeeded.
