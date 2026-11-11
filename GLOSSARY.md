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

## Character

**Character**:
An AI persona profile that can be chatted with, made of a name, description, greetings and prompt fields.
_Avoid_: persona, bot, agent

**Long Text Field**:
A multi-line character field that starts at one line, grows with its content, and scrolls internally after reaching its field-specific limit.
_Avoid_: Text box, text area

**Max Lines**:
The maximum number of natural text lines a Long Text Field displays before it stops growing; it is a height limit, not a character limit.
_Avoid_: Max length, character limit

**Alternate Greeting**:
A variant opening message that can be selected instead of the character's first message; one character may have several.
_Avoid_: Backup greeting, extra first message

## World Info

**World Info Book** (世界书文集):
A named collection of World Info Entries; it is the unit of binding and of enabling.
_Avoid_: 世界书, book, library, pack

**World Info Entry** (世界书条目):
A single piece of knowledge inside a World Info Book, carrying its own trigger rules; when a rule matches, its content is injected into the prompt.
_Avoid_: entry card, rule, node

**Enabled** (启用):
Whether a World Info Book or a World Info Entry takes part in injection; it is a property of that item itself.
_Avoid_: active, on, checked

**Character Book Binding** (角色世界书绑定):
The link between a Character and one World Info Book, marking the book that character's chats are meant to use. A character holds at most one binding; without one it is unbound.
_Avoid_: association, mount, reference

**Unbind** (解绑):
Removing a Character Book Binding so the character becomes unbound. Unbinding neither deletes the World Info Book nor its entries.
_Avoid_: remove, delete, disable

**World Info Injection** (世界信息注入):
The step before each generation that selects entries from World Info Books by their trigger rules and inserts them into the prompt context.
_Avoid_: world book injection, RAG, retrieval
