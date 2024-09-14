
# foo_lock

[foo_lock](https://github.com/stuerp/foo_lock/releases) is a [foobar2000](https://www.foobar2000.org/) component that performs a playback action
when the workstation is locked or unlocked or the session is disconnected or reconnected.

It is based on [foo_lock](https://www.foobar2000.org/components/view/foo_lock) by [kode54](https://www.foobar2000.org/components/author/kode54).

To use it, specify the playback action that should be taken when the session locks or unlocks in the Advanced branch of the Preferences dialog. Look for "foo_lock" under the "Playback" branch.

After doing that you can enable or disable the component using the "Playback / Pause on lock" menuitem.

## Requirements

* Tested on Microsoft Windows 10 and later.
* [foobar2000](https://www.foobar2000.org/download) v1.6.16 or later (32 or 64-bit). ![foobar2000](https://www.foobar2000.org/button-small.png)

## Getting started

* Double-click `foo_lock.fbk2-component`.

or

* Import `foo_lock.fbk2-component` into foobar2000 using "File / Preferences / Components / Install...".

## Developing

### Building

Open `foo_lock.sln` with Visual Studio and build the solution.

### Packaging

To create the component first build the x86 configuration and next the x64 configuration.

## Change Log

v0.8.1.1, 2024-09-14

* Updated README with instructions on how to use the component.

v0.8.1.0, 2023-11-06

* Added x64 build for foobar2000 2.0 and later

## License

![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)
