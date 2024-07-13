# Markdown

A customizable, 0 dependency markdown compiler written in C

This implementation isn't following a specification in particular, rather it's a mix of useful features I like. For now it has:

- Paragraphs
- Headings
- Links
- Unordered lists
- Asides
- Code blocks ([partial syntax highlighting](#code-blocks))

## Customization

You may customize some features of the compiler through the `config.h` file, check out the [sample config file](./config.def.h) I've provided.

### Code blocks

Code blocks compile into an HTML [pre](https://developer.mozilla.org/en-US/docs/Web/HTML/Element/pre) element. Each child gets a specific class depending on its nature:

| Token type       | Class   |
|------------------|---------|
| String           | `c-s`   |
| Keyword          | `c-k`   |

If you want the compiler to identify keywords, you'll need to add a list of keywords associated with each language you intend to use.
