// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import {
  InterfaceDeclaration,
  ClassDeclaration,
  MethodDeclaration,
  MethodSignature,
  SyntaxKind,
  PropertyDeclaration,
  PropertySignature,
  EnumDeclaration,
  EnumMember,
  SourceFile,
  ConstructorDeclaration,
  JSDoc,
  GetAccessorDeclaration,
  TypeAliasDeclaration,
} from 'ts-morph';
import {
  BaseMember,
  BaseMemberType,
  BaseObject,
  BaseObjectType,
  BaseParam,
} from './metadata-def';

function isNeedParse(
  obj:
    | ClassDeclaration
    | InterfaceDeclaration
    | EnumDeclaration
    | TypeAliasDeclaration,
  export_object: string[]
): boolean {
  if (obj.isExported()) {
    const name = obj.getName();
    if (name && export_object.includes(name)) {
      return true;
    }
  }
  return false;
}

// Function to build class prototype
export function buildClassPrototype(classDecl: ClassDeclaration): BaseObject {
  let classObject: BaseObject = {
    name: classDecl.getName()!,
    type: BaseObjectType.ClassType,
    brief_desc: '',
    detailed_desc: '',
    definition: '',
    has_apidoc: false,
    language: 'typescript',
    since: '',
    children: [],
  };

  handleJSDoc(classObject, classDecl.getJsDocs());

  // Get modifiers (e.g., export, abstract)
  const modifiers = classDecl
    .getModifiers()
    .map((mod) => mod.getText())
    .join(' ');

  // Get type parameters (generics)
  const typeParameters = classDecl
    .getTypeParameters()
    .map((tp) => tp.getText())
    .join(', ');
  const genericPart = typeParameters ? `<${typeParameters}>` : '';

  // Get base class (extends)
  const baseClass = classDecl.getExtends()?.getText() || '';
  const extendsPart = baseClass ? ` extends ${baseClass}` : '';

  // Get implemented interfaces
  const interfaces = classDecl
    .getImplements()
    .map((impl) => impl.getText())
    .join(', ');
  const implementsPart = interfaces ? ` implements ${interfaces}` : '';

  // Get class definition
  classObject.definition = `${modifiers} class ${classObject.name}${genericPart}${extendsPart}${implementsPart}`;
  return classObject;
}

function handleJSDoc(member: BaseMember | BaseObject, jsDocs: JSDoc[]): void {
  member.brief_desc = jsDocs
    .map((desc) => desc.getDescription().trim())
    .join('')
    .replaceAll('\n', ' ');
  member.has_apidoc = jsDocs.some((doc) =>
    doc.getTags().some((tag) => tag.getTagName() === 'apidoc')
  );
  if (!('params' in member) || !('returns' in member)) {
    return;
  }

  let paramsIndex = 0;
  jsDocs.forEach((doc) => {
    doc.getTags().forEach((tag) => {
      const tagName = tag.getTagName();
      const comment = tag.getCommentText()?.toString() || '';

      if (tagName === 'param') {
        member.params[paramsIndex++].brief_desc = comment.replaceAll('\n', ' ');
      } else if (tagName === 'returns' || tagName === 'return') {
        member.returns!.brief_desc = comment.replaceAll('\n', ' ');
      } else if (tagName === 'since') {
        member.since = comment;
      }
    });
  });
}

export function buildGetterPrototype(
  getter: GetAccessorDeclaration
): BaseMember {
  const name = getter.getName();
  const returnType = getter.getReturnTypeNode()?.getText() ?? 'any';

  const getterObject: BaseMember = {
    name: name,
    type: BaseMemberType.MethodType,
    brief_desc: '',
    detailed_desc: '',
    definition: `get ${name}(): ${returnType}`,
    prototype: `get ${name}(): ${returnType}`,
    has_apidoc: false,
    params: [],
    returns: {
      name: '',
      type: returnType,
      brief_desc: '',
    },
    note: [],
    info: [],
    caution: [],
    warning: [],
    since: '',
  };

  handleJSDoc(getterObject, getter.getJsDocs());

  return getterObject;
}

// Function to build method prototype
export function buildMethodPrototype(
  method: MethodDeclaration | MethodSignature | GetAccessorDeclaration
): BaseMember {
  let methodObject: BaseMember = {
    name: method.getName()!,
    type: BaseMemberType.MethodType,
    brief_desc: '',
    detailed_desc: '',
    definition: '',
    prototype: '',
    has_apidoc: false,
    params: [],
    returns: null,
    note: [],
    info: [],
    caution: [],
    warning: [],
    since: '',
  };
  const parameters = method
    .getParameters()
    .map((param) => {
      const paramName = param.getName();
      const paramType = param.getTypeNode()?.getText() ?? '';
      methodObject.params.push({
        name: paramName,
        type: paramType,
        brief_desc: '',
      });
      return `${paramName}: ${paramType}`;
    })
    .join(', ');

  const returnType = method.getReturnTypeNode()?.getText() ?? 'void';
  methodObject.returns = {
    name: '',
    type: returnType,
    brief_desc: '',
  };

  handleJSDoc(methodObject, method.getJsDocs());

  let modifiers = '';
  if (method instanceof MethodDeclaration) {
    modifiers = method
      .getModifiers()
      .map((mod) => mod.getText())
      .join(' ');
  }

  const typeParameters = method
    .getTypeParameters()
    .map((tp) => tp.getText())
    .join(', ');
  const genericPart = typeParameters ? `<${typeParameters}>` : '';
  const modifierStr = modifiers ? modifiers + ' ' : '';
  methodObject.definition = `${modifierStr}${methodObject.name}${genericPart}(${parameters}): ${returnType};`;
  methodObject.prototype = methodObject.definition;

  return methodObject;
}

// Function to build interface prototype
export function buildInterfacePrototype(
  interfaceDecl: InterfaceDeclaration
): BaseObject {
  let interfaceObject: BaseObject = {
    name: interfaceDecl.getName()!,
    type: BaseObjectType.InterfaceType,
    brief_desc: '',
    detailed_desc: '',
    definition: '',
    has_apidoc: false,
    language: 'typescript',
    children: [],
    since: '',
  };
  // Get modifiers (e.g., export, abstract)
  const modifiers = interfaceDecl
    .getModifiers()
    .map((mod) => mod.getText())
    .join(' ');
  // Get type parameters (generics)
  const typeParameters = interfaceDecl
    .getTypeParameters()
    .map((tp) => tp.getText())
    .join(', ');
  const genericPart = typeParameters ? `<${typeParameters}>` : '';
  // Get interface definition
  interfaceObject.definition = `${modifiers} interface ${interfaceObject.name}${genericPart}`;
  return interfaceObject;
}

export function buildEnumPrototype(enumDecl: EnumDeclaration): BaseObject {
  let enumObject: BaseObject = {
    name: enumDecl.getName()!,
    type: BaseObjectType.EnumType,
    brief_desc: '',
    detailed_desc: '',
    definition: '',
    has_apidoc: false,
    language: 'typescript',
    since: '',
    children: [],
  };
  // Get modifiers (e.g., export)
  const modifiers = enumDecl
    .getModifiers()
    .map((mod) => mod.getText())
    .join(' ');

  // Get enum definition
  enumObject.definition = `${modifiers} enum ${enumObject.name}`;
  return enumObject;
}

export function buildEnumMemberPrototype(member: EnumMember): BaseMember {
  let memberObject: BaseMember = {
    name: member.getName(),
    type: BaseMemberType.PropertyType,
    brief_desc: '',
    detailed_desc: '',
    definition: '',
    prototype: '',
    has_apidoc: false,
    params: [],
    returns: null,
    note: [],
    info: [],
    caution: [],
    warning: [],
    since: '',
  };

  const name = member.getName();
  const initializer = member.getInitializer()?.getText();
  memberObject.definition = initializer ? `${name} = ${initializer}` : name;
  memberObject.prototype = memberObject.definition;

  return memberObject;
}

export function buildPropertyPrototype(
  propertyDecl: PropertyDeclaration | PropertySignature
): BaseMember {
  let propertyObject: BaseMember = {
    name: propertyDecl.getName(),
    type: BaseMemberType.PropertyType,
    brief_desc: '',
    detailed_desc: '',
    definition: '',
    prototype: '',
    has_apidoc: false,
    params: [],
    returns: null,
    note: [],
    info: [],
    caution: [],
    warning: [],
    since: '',
  };
  let modifiers = '';
  let initializer: string | undefined;

  if (propertyDecl instanceof PropertyDeclaration) {
    modifiers = propertyDecl
      .getModifiers()
      .map((mod) => mod.getText())
      .join(' ');
    initializer = propertyDecl.getInitializer()?.getText();
  } else {
    // PropertySignature can have readonly keyword
    if (propertyDecl.hasModifier(SyntaxKind.ReadonlyKeyword)) {
      modifiers = 'readonly';
    }
  }

  const name = propertyDecl.getName();
  const type = propertyDecl.getTypeNode()?.getText() ?? 'any';
  const isOptional = propertyDecl.hasQuestionToken() ? '?' : '';

  const modifierPart = modifiers ? `${modifiers} ` : '';
  let definition = `${modifierPart}${name}${isOptional}: ${type};`;
  if (initializer) {
    definition += ` = ${initializer}`;
  }
  // Use trim to handle cases with no modifiers
  propertyObject.definition = definition.trim();
  propertyObject.prototype = propertyObject.definition;
  return propertyObject;
}

export function buildConstructorPrototype(
  constructor: ConstructorDeclaration
): BaseMember {
  let constructorObject: BaseMember = {
    name: 'constructor',
    type: BaseMemberType.ConstructorType,
    brief_desc: '',
    detailed_desc: '',
    definition: '',
    prototype: '',
    has_apidoc: false,
    params: [],
    returns: null,
    note: [],
    info: [],
    caution: [],
    warning: [],
    since: '',
  };

  const parameters = constructor
    .getParameters()
    .map((param) => {
      const paramName = param.getName();
      const paramType = param.getTypeNode()?.getText() ?? '';
      constructorObject.params.push({
        name: paramName,
        type: paramType,
        brief_desc: '',
      });
      return `${paramName}: ${paramType}`;
    })
    .join(', ');
  constructorObject.returns = {
    name: '',
    type: 'void',
    brief_desc: '',
  };

  handleJSDoc(constructorObject, constructor.getJsDocs());

  let modifiers = '';
  if (constructor instanceof ConstructorDeclaration) {
    modifiers = constructor
      .getModifiers()
      .map((mod) => mod.getText())
      .join(' ');
  }

  const modifierStr = modifiers ? modifiers + ' ' : '';
  constructorObject.definition = `${modifierStr}constructor(${parameters});`;
  constructorObject.prototype = constructorObject.definition;

  return constructorObject;
}

export function buildTypeAliasPrototype(
  typeAliasDecl: TypeAliasDeclaration
): BaseObject {
  const name = typeAliasDecl.getName();

  const modifiers = typeAliasDecl
    .getModifiers()
    .map((mod) => mod.getText())
    .join(' ');
  const typeParameters = typeAliasDecl
    .getTypeParameters()
    .map((tp) => tp.getText())
    .join(', ');
  const genericPart = typeParameters ? `<${typeParameters}>` : '';
  const typeText = typeAliasDecl.getTypeNode()?.getText();
  const definition = `${modifiers} type ${name}${genericPart} = ${typeText}`;

  let typeAliasObject: BaseObject = {
    name: name,
    type: BaseObjectType.TypeAliasType,
    brief_desc: '',
    detailed_desc: '',
    definition: definition,
    has_apidoc: false,
    language: 'typescript',
    since: '',
    children: [],
  };

  return typeAliasObject;
}

export function parseSourceFile(
  sourceFile: SourceFile,
  export_object: string[]
): BaseObject[] {
  const metadata: BaseObject[] = [];

  // Process classes
  sourceFile.getClasses().forEach((classDecl) => {
    if (isNeedParse(classDecl, export_object)) {
      const classObject = buildClassPrototype(classDecl);
      classDecl.getConstructors().forEach((constructor) => {
        if (
          !constructor.hasModifier(SyntaxKind.PrivateKeyword) &&
          !constructor.hasModifier(SyntaxKind.ProtectedKeyword)
        ) {
          classObject.children.push(buildConstructorPrototype(constructor));
        }
      });
      classDecl.getMethods().forEach((method) => {
        if (
          !method.hasModifier(SyntaxKind.PrivateKeyword) &&
          !method.hasModifier(SyntaxKind.ProtectedKeyword)
        ) {
          classObject.children.push(buildMethodPrototype(method));
        }
      });
      classDecl.getProperties().forEach((prop) => {
        if (
          !prop.hasModifier(SyntaxKind.PrivateKeyword) &&
          !prop.hasModifier(SyntaxKind.ProtectedKeyword)
        ) {
          classObject.children.push(buildPropertyPrototype(prop));
        }
      });
      classDecl.getGetAccessors().forEach((getter) => {
        if (
          !getter.hasModifier(SyntaxKind.PrivateKeyword) &&
          !getter.hasModifier(SyntaxKind.ProtectedKeyword)
        ) {
          classObject.children.push(buildMethodPrototype(getter));
        }
      });
      metadata.push(classObject);
    }
  });

  // Process interfaces
  sourceFile.getInterfaces().forEach((interfaceDecl) => {
    if (isNeedParse(interfaceDecl, export_object)) {
      const interfaceObject = buildInterfacePrototype(interfaceDecl);
      interfaceDecl.getMethods().forEach((method) => {
        interfaceObject.children.push(buildMethodPrototype(method));
      });
      interfaceDecl.getProperties().forEach((prop) => {
        interfaceObject.children.push(buildPropertyPrototype(prop));
      });
      metadata.push(interfaceObject);
    }
  });

  // Process enums
  sourceFile.getEnums().forEach((enumDecl) => {
    if (isNeedParse(enumDecl, export_object)) {
      const enumObject = buildEnumPrototype(enumDecl);
      enumDecl.getMembers().forEach((member) => {
        enumObject.children.push(buildEnumMemberPrototype(member));
      });
      metadata.push(enumObject);
    }
  });

  // Process exported variable declarations that are class instances
  sourceFile.getVariableStatements().forEach((statement) => {
    if (statement.isExported()) {
      statement.getDeclarations().forEach((varDecl) => {
        const initializer = varDecl.getInitializer();
        if (initializer && initializer.getKind() === SyntaxKind.NewExpression) {
          const newExpression = initializer.asKindOrThrow(
            SyntaxKind.NewExpression
          );
          const identifier = newExpression.getExpression();

          if (identifier.getKind() === SyntaxKind.Identifier) {
            const className = identifier.getText();
            const classDecl = sourceFile.getClass(className);

            if (classDecl) {
              const classObject = buildClassPrototype(classDecl);
              classObject.name = varDecl.getName();

              const typeName = classDecl.getName() ?? 'any';
              classObject.definition = `export const ${varDecl.getName()}: ${typeName}`;

              classDecl.getMethods().forEach((method) => {
                if (
                  !method.hasModifier(SyntaxKind.PrivateKeyword) &&
                  !method.hasModifier(SyntaxKind.ProtectedKeyword)
                ) {
                  classObject.children.push(buildMethodPrototype(method));
                }
              });
              classDecl.getProperties().forEach((prop) => {
                if (
                  !prop.hasModifier(SyntaxKind.PrivateKeyword) &&
                  !prop.hasModifier(SyntaxKind.ProtectedKeyword)
                ) {
                  classObject.children.push(buildPropertyPrototype(prop));
                }
              });
              metadata.push(classObject);
            }
          }
        }
      });
    }
  });

  sourceFile.getTypeAliases().forEach((typeAlias) => {
    if (isNeedParse(typeAlias, export_object)) {
      metadata.push(buildTypeAliasPrototype(typeAlias));
    }
  });

  return metadata;
}
