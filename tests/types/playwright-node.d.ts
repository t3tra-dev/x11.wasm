declare module "node:fs" {
  export function readFileSync(path: string, encoding: string): string;
  export function accessSync(path: string, mode: number): void;
  export const constants: { X_OK: number };
  export function mkdirSync(path: string, options: { recursive: boolean; mode?: number }): string | undefined;
  export function mkdtempSync(prefix: string): string;
  export function writeFileSync(path: string, data: string, options: { mode: number; flag: string }): void;
}

declare module "node:path" {
  export function resolve(...paths: string[]): string;
  export function join(...paths: string[]): string;
  export function isAbsolute(path: string): boolean;
}

declare var __dirname: string;
declare var process: {
  env: Record<string, string | undefined>;
  platform: string;
};

declare module "@playwright/test" {
  export interface PlaywrightTestConfig {
    testDir?: string;
    timeout?: number;
    webServer?: {
      command?: string;
      cwd?: string;
      url?: string;
      reuseExistingServer?: boolean;
      timeout?: number;
    };
    use?: Record<string, unknown>;
  }

  export interface ConsoleMessage {
    type(): string;
    text(): string;
  }

  export interface Locator {
    click(options?: unknown): Promise<void>;
    focus(options?: unknown): Promise<void>;
    textContent(): Promise<string | null>;
    getAttribute(name: string): Promise<string | null>;
    boundingBox(): Promise<{ x: number; y: number; width: number; height: number } | null>;
    evaluate<Result>(callback: (element: HTMLElement) => Result | Promise<Result>): Promise<Awaited<Result>>;
    evaluate<Arg, Result>(callback: (element: HTMLElement, arg: Arg) => Result | Promise<Result>, arg: Arg): Promise<Awaited<Result>>;
  }

  export interface Page {
    goto(url: string): Promise<unknown>;
    locator(selector: string): Locator;
    evaluate<Result>(
      pageFunction: (() => Result) | (() => Promise<Result>) | string,
    ): Promise<Awaited<Result>>;
    evaluate<Arg, Result>(
      pageFunction:
        | ((arg: Arg) => Result)
        | ((arg: Arg) => Promise<Result>)
        | string,
      arg: Arg,
    ): Promise<Awaited<Result>>;
    on(event: "console", handler: (message: ConsoleMessage) => void): void;
    on(event: "pageerror", handler: (error: Error) => void): void;
    setContent(html: string): Promise<void>;
    addScriptTag(options: { path?: string; url?: string; content?: string }): Promise<unknown>;
    addInitScript(script: (() => void) | string): Promise<void>;
    waitForFunction(
      pageFunction: (() => unknown) | string,
      arg?: unknown,
      options?: unknown,
    ): Promise<unknown>;
  }

  export interface TestFixtures {
    page: Page;
  }

  export interface Matchers {
    not: Matchers;
    toBe(expected: unknown): unknown;
    toBeCloseTo(expected: number, precision?: number): unknown;
    toBeGreaterThan(expected: number): unknown;
    toBeGreaterThanOrEqual(expected: number): unknown;
    toBeLessThan(expected: number): unknown;
    toBeLessThanOrEqual(expected: number): unknown;
    toBeNull(): unknown;
    toBeFocused(options?: unknown): Promise<void>;
    toBeVisible(options?: unknown): Promise<void>;
    toBeHidden(options?: unknown): Promise<void>;
    toBeTruthy(): unknown;
    toContain(expected: unknown): unknown;
    toEqual(expected: unknown): unknown;
    toHaveAttribute(name: string, value: string | RegExp, options?: unknown): Promise<void>;
    toHaveClass(value: string | RegExp, options?: unknown): Promise<void>;
    toHaveCount(count: number, options?: unknown): Promise<void>;
    toHaveLength(length: number): unknown;
    toHaveText(value: string | RegExp, options?: unknown): Promise<void>;
    toMatchObject(expected: unknown): unknown;
  }

  export interface Expect {
    (actual: unknown, message?: string): Matchers;
    poll(actual: () => unknown | Promise<unknown>, options?: {
      timeout?: number; intervals?: number[];
    }): Matchers;
    any(constructor: unknown): unknown;
    objectContaining(value: unknown): unknown;
  }

  export interface Test {
    (
      name: string,
      fn: (fixtures: TestFixtures) => unknown | Promise<unknown>,
    ): void;
    fixme(
      name: string,
      fn: (fixtures: TestFixtures) => unknown | Promise<unknown>,
    ): void;
  }

  export const expect: Expect;
  export const test: Test;
}

declare var module: {
  exports: unknown;
};
