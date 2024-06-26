// Automatically generated by GDevelop.js/scripts/generate-types.js
declare class gdGroupEvent extends gdBaseEvent {
  constructor(): void;
  setName(name: string): void;
  getName(): string;
  setBackgroundColor(r: number, g: number, b: number): void;
  getBackgroundColorR(): number;
  getBackgroundColorG(): number;
  getBackgroundColorB(): number;
  setSource(source: string): void;
  getSource(): string;
  getCreationParameters(): gdVectorString;
  getCreationTimestamp(): number;
  setCreationTimestamp(ts: number): void;
  delete(): void;
  ptr: number;
};