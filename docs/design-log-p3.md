# Project 3 Design Log
**Author:** Ladi Folarin  
**P2 code used:** My own Project 2 implementation. I carried forward `conversation.h/.cpp` and `sentinel_scanner.h/.cpp` from my completed P2 submission as required by the P3 specification.  
**Tools used:** ChatGPT was used to help interpret the specification, implement and review the three required P3 components, expand the test suite, and diagnose build/test issues. CMake, the C++ compiler, AddressSanitizer/UndefinedBehaviorSanitizer, and CTest were used to build and verify the program.

## 1. Hash table

I implemented `ToolRegistry` with **separate chaining**. The table is a `std::vector` of buckets, and each bucket is a `std::vector<std::unique_ptr<Tool>>`. I chose chaining instead of linear probing because the registry never needs deletion and the specification puts strong emphasis on pointer stability. Chaining makes collision handling straightforward and avoids the clustering behavior that linear probing can develop as the table fills.

For hashing I used the 64-bit **FNV-1a** algorithm from Appendix A. It examines every character and is order-sensitive, so names such as `ab` and `ba` do not normally collide just because they contain the same characters. I start with 8 buckets and resize when the next insertion would make the load factor exceed **0.75**. Resizing doubles the bucket count and rehashes every existing tool using the new count.

The 10,000-tool test produced these measurements:

- bucket count: **16,384**
- load factor: **0.610352**
- maximum chain length: **6**
- weak alternative hash (`s[0] + s.size()`) maximum chain: **9,000**

The comparison demonstrates why a real string hash matters. The weak hash sees almost no meaningful difference among names such as `tool_1` through `tool_9999`, while FNV-1a spreads them across the table. `add()` is amortized O(1): ordinary inserts are expected O(1), while an occasional resize costs O(n). Because the table doubles, many cheap inserts occur between resize operations, so the resize cost is spread across those inserts.

## 2. Pointer stability

The registry owns every tool with `std::unique_ptr<Tool>`. During a resize I move the `unique_ptr`s into new buckets, but I never move or copy the actual `Tool` objects they point to. A `unique_ptr` stores an address; moving the `unique_ptr` transfers ownership of that same address. Therefore a `Tool*` returned earlier by `find()` still points to the same object after the table is rehashed. My 10,000-tool test saves the pointer for `tool_0`, forces many resizes, and verifies that `find("tool_0")` still returns exactly the original pointer.

## 3. Calculator trace

For `2 * (3 + 4) - 5`, I use one stack for values and one for operators. Reading `2` pushes 2 onto `values`. `*` goes onto `ops`. `(` is then pushed as a boundary. Reading `3` pushes 3; `+` is pushed because the operator above it is `(`; and `4` pushes 4. When `)` arrives, I apply `+`: I pop right operand 4, then left operand 3, compute 7, push 7, and remove the `(`. The stacks now contain values `[2, 7]` and operator `*`.

When `-` arrives, the existing `*` has higher precedence, so I apply it first and push 14. Then I push `-`. Reading `5` pushes 5. At end of input I apply the remaining subtraction: 14 - 5 = 9. The single value left is the answer.

## 4. Errors as values

Tool failures are expected events, not necessarily programming bugs. A requested file may not exist, a model may request an unsafe path, or a calculation may divide by zero. Returning `false` plus a message lets the harness place that failure back into the conversation so the model can react to it instead of terminating the program. Exceptions are more appropriate for programmer errors or violated internal assumptions, such as calling the wrong `JsonValue` accessor. This keeps recoverable model/tool errors separate from actual defects in the C++ program.

## 5. What I would design differently

If I were designing the public interface myself, I would return a small result object from `resolve` and `calc_evaluate` instead of using a Boolean plus output parameters. A result type containing either a value or an error message would make success and failure harder to mix up and would make the calling code more self-documenting. I would keep the same hash-table ownership approach because it was simple and gave strong pointer stability and good measured performance.
