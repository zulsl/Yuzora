# Yuzora

Yuzora 的领域语言，围绕角色、会话、提示词预设和世界书展开。

## Language

**Prompt Entry**:
A configurable instruction entry belonging to a completion preset, identified by a stable `identifier` rather than its display name.
_Avoid_: Prompt row, prompt item

**Prompt Identifier**:
The stable key of a Prompt Entry; it is what Prompt Order stores and must not change when the entry is renamed.
_Avoid_: Prompt id, name

**Prompt Order**:
The ordered list of Prompt Identifiers for one preset; it is the authoritative display and execution order.
_Avoid_: Sort order, prompt list order

**System Prompt**:
A Prompt Entry marked as built-in and protected from deletion.
_Avoid_: Locked prompt, built-in prompt

**Marker**:
A Prompt Entry that marks a structural insertion point, such as chat history; a Marker is not automatically a System Prompt.
_Avoid_: Placeholder, anchor

**Injection Order**:
A prompt's injection priority inside its target position; it is independent of Prompt Order and must not be renumbered when Prompt Order changes.
_Avoid_: Prompt order, list order

**Prompt Insertion**:
Placing a new Prompt Entry immediately before the selected Prompt Entry in Prompt Order.
_Avoid_: Append, add to end

**Prompt Addition**:
Appending a new Prompt Entry to the end of Prompt Order.
_Avoid_: Insertion, insert before
