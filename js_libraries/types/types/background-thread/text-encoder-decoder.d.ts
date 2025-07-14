export interface TextEncoder {
    encode(str: string): ArrayBuffer;
    encoding: 'utf-8';
}

type TypedArray =
  | Int8Array
  | Uint8Array
  | Uint8ClampedArray
  | Int16Array
  | Uint16Array
  | Int32Array
  | Uint32Array
  | Float32Array
  | Float64Array;

export interface TextDecoder {
    decode(buffer: ArrayBuffer | TypedArray | DataView): string;
    encoding: 'utf-8';
    fatal: false;
    ignoreBOM: true;
}