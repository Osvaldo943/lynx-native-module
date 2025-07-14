// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface BaseParam {
  name: string;
  type: string;
  brief_desc: string;
}

export enum BaseMemberType {
  MethodType = 'method',
  PropertyType = 'property',
  ConstructorType = 'constructor',
}

export interface BaseMember {
  name: string;
  type: BaseMemberType;
  brief_desc: string;
  detailed_desc: string;
  definition: string;
  prototype: string;
  has_apidoc: boolean;
  params: BaseParam[];
  returns: BaseParam | null;
  note: string[];
  info: string[];
  caution: string[];
  warning: string[];
  since: string;
}

export enum BaseObjectType {
  ClassType = 'class',
  InterfaceType = 'interface',
  EnumType = 'enum',
  TypeAliasType = 'type',
}

export interface BaseObject {
  name: string;
  type: BaseObjectType;
  brief_desc: string;
  detailed_desc: string;
  definition: string;
  has_apidoc: boolean;
  language: string;
  since: string;
  children: BaseMember[];
}
