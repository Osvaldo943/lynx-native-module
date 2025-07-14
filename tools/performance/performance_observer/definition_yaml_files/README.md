# PerformanceEntry Definitions

## Description

The YAML files in this directory are intended to define [PerformanceEntry](https://lynxjs.org/api/lynx-api/performance-api/performance-entry.html). The `generate_performance_entry.py` script utilizes these YAML files to generate corresponding files in Java, Objective-C and TypeScript.

## Usage

- **How to modify a PerformanceEntry?**

_Modifying an Existing PerformanceEntry_: Simply update the relevant YAML file.

_Adding a New PerformanceEntry_: Create a new YAML file and place it in the `lynx/tools/performance/performance_observer/definition_yaml_files`.

- **How can I verify my modifications?**

Generate PerformanceEntry files through habitat sync:

```shell
cd /path/to/lynx
source ./tools/envsetup.sh
./hab sync
```

Then verify the output:

1. Verify `lynx/js_libraries/types/types/background-thread` for TypeScript interfaces.
2. Verify `lynx/platform/android/lynx_android/src/main/java/com/lynx/tasm/performance/performanceobserver` for Java files.
3. Verify `lynx/platform/darwin/common/lynx/performance/performance_observer` for Objective-C implementation files and `lynx/platform/darwin/common/lynx/public/performance/performance_observer` for Objective-C header files.

### Example

Here's an example of adding a new PerformanceEntry, such as **TestCaseEntry**.

```typescript
export interface TestCaseResult {
    status: number;
    isSuccess: boolean;
    errorMessage: string;
}

export interface TestCaseEntry extends PerformanceEntry {
    testStart: number;
    testEnd: number;
    testInfo: Record<string, number>;
    testResult: TestCaseResult;
}
```

1. Create a new YAML file named `TestCaseEntry.yml` in the `lynx/tools/performance/performance_observer/definition_yaml_files` directory.

2. Add the following content to the `TestCaseEntry.yml` file:

```yaml
TestCaseResult:
  type: object
  properties:
    status:
        type: number
    isSuccess:
        type: boolean
    errorMessage:
        type: string

TestCaseEntry:
  allOf:
    - $ref: 'PerformanceEntry.yml#/PerformanceEntry'
    - type: object
      properties:
        testStart:
            type: number
        testEnd:
            type: number
        testInfo:
            type: map
            keyType: string
            valueType: number
        testResult:
            $ref: '#/TestCaseResult'
```

### YAML Syntax

A PerformanceEntry file should include a new class name, inheritances, and variable definitions. Its structure typically looks like this:

```yaml
# NewEntry.yml
NewEntry:
  allOf:
    - $ref: 'PerformanceEntry.yml#/PerformanceEntry'
    - type: object
      properties:
        propName:
            type: string
```

`allOf` signifies that your definition inherits from `PerformanceEntry`, so ensure that `NewEntry` maintains this inheritance relationship.

#### Variable Definitions

Variables are defined using the `properties` section. Each variable should have a name and a type. You can use `type` and `$ref` to define the type of the variable.

Now the following types are supported:

- number

- integer

- string

- boolean

- timestamp

- map

- ref

You can define basic data types as follows:

```yaml
name:
    type: number/string/boolean/timestamp/integer
```

For map types, you should specify the key and value types:

```yaml
name:
    type: map
    keyType: string
    valueType: string
```

For reference types, you can use the `$ref` tag:

```yaml
name:
    $ref: 'ReferenceObject.yml#/ReferenceObject'
```

#### Extension Tags

To effectively manage multi-platform outputs, we've introduced these symbols to control the generation of scripts exclusively.

- `x-ts-only`: This tag indicates that the script should only be generated for TypeScript.

For example, by using the `x-ts-only` tag in `TestCaseResult` of TestCaseEntry.yml, **only TestCaseEntry.ts** will have definiton of TestCaseResult, without producing scripts for Java, ObjectC, or any other languages.

```yaml
# TestCaseEntry.yml
TestCaseResult:
  x-ts-only: true
  type: object
  properties:
    status:
        type: number
    isSuccess:
        type: boolean
    errorMessage:
        type: string
```
