import assert from "node:assert";
import { test } from "node:test";

test("can load grammar", async () => {
  const binding = (await import("./index.mjs")).default;
  assert.ok(binding, "binding should load");
  assert.ok(binding.language, "binding should export language");
});

test("has node type info", async () => {
  const binding = (await import("./index.mjs")).default;
  assert.ok(binding.nodeTypeInfo, "should have node type info");
  assert.ok(Array.isArray(binding.nodeTypeInfo), "nodeTypeInfo should be array");
  assert.ok(binding.nodeTypeInfo.length > 0, "nodeTypeInfo should not be empty");
});

test("has query files", async () => {
  const binding = (await import("./index.mjs")).default;
  assert.ok(binding.HIGHLIGHTS_QUERY, "should have highlights query");
  assert.ok(binding.HIGHLIGHTS_QUERY.includes("@keyword"), "highlights should contain @keyword");
  assert.ok(binding.INJECTIONS_QUERY, "should have injections query");
  assert.ok(binding.INJECTIONS_QUERY.includes("bash"), "injections should reference bash");
  assert.ok(binding.LOCALS_QUERY, "should have locals query");
  assert.ok(binding.TAGS_QUERY, "should have tags query");
});
