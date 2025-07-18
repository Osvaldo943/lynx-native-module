# Building Lynx Explorer for iOS

This document provides instructions for building the Lynx Explorer iOS app from source. If you just want to try out Lynx, you can download the pre-built app from the releases page instead.

## System Requirements

- 100GB or more of disk space
- Git/Python3(>=3.9)/Ruby(>=2.7, <3.4) installed

## Install Dependencies

The following dependencies are needed:

- Xcode(>=15.0)
- Python library

We recommend using [Homebrew](https://brew.sh/) to install the dependencies.

### Xcode

Lynx requires Xcode 15.0 or later. It is recommended to keep Xcode up to date. You can install or update it on the [App Store](https://developer.apple.com/xcode/).

- Open Xcode->Settings->Locations, to make sure the `Command Line Tools` are configured
- You can run `xcode-select -p` in the terminal, and if it prints a correct path, it's configure successfully.

### Python Library

The yaml dependency needs to be installed to execute some auto-generation logic.

```
# use the virtual environment to manage python environment
python3 -m venv venv
source venv/bin/activate
# install PyYAML package
pip3 install pyyaml
```

## Get the Source Code

### Pull the Repository

Pull the code from the Github repository.

```
git clone https://github.com/lynx-family/lynx.git
```

### Install Third-party Library

Run the following commands from the root of the repository to install the dependencies.

```
cd lynx
tools/hab sync .
source tools/envsetup.sh
```

## Build iOS App

1. Install iOS project dependencies
```
cd explorer/darwin/ios/lynx_explorer
./bundle_install.sh
```
2. After step 1, `LynxExplorer.xcworkspace` will be generated in the lynx_explorer directory. Open `LynxExplorer.xcworkspace` by Xcode.
3. Select `LynxExplorer` to execute the build in Xcode.

## Troubleshooting

### Using a Personal Team to Run on Device

By default, the project doesn't configure the "Team" for signing. If you want to change it to your Personal Team (associated with your Apple ID), follow these steps:

1. Open `LynxExplorer.xcworkspace` by Xcode.
2. Navigate to "Signing & Capabilities", and select your Personal Team under the "Team" dropdown.
3. Update the "Bundle Identifier" from `com.lynx.LynxExplorer` to a unique identifier like `com.<your-name>.LynxExplorer`. This step ensures the identifier is unique and available for your use.
4. Enable the "Automatically manage signing" option to allow Xcode to handle the app signing process automatically.

### Handle Error when Installing Ruby Gems

When running `./bundle_install.sh` command, if you encounter the following problems: *Bundler::GemNotFound: Your bundle is lock to xxx, but that version could not be found in any of the sources listed in Gemfile...* You can try running the following commands to fix the problem:
```
SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk bundle install
./bundle_install.sh
```
