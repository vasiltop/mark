#pragma once

#include "core.hpp"
#include <variant>

namespace ir {

struct InlineNode;

struct Text {
  const char *start;
  s32 len;
};

struct Strong {
  InlineNode *content;
};

struct Emph {
  InlineNode *content;
};

struct Link {
  const char *url;
  InlineNode *label;
};

struct Image {
  const char *src;
  const char *alt;
};

struct Ref {
  const char *label;
};

struct FuncCall {
  const char *name;
  InlineNode *content;
};

using InlineItem = std::variant<Text, Strong, Emph, Link, Image, Ref, FuncCall>;

struct InlineNode {
  InlineItem item;
  InlineNode *next;
};

struct Heading {
  s32 level;
  InlineNode *content;
  const char *label;
};

struct Paragraph {
  InlineNode *content;
};

struct ListItem {
  InlineNode *content;
  ListItem *next;
};

struct List {
  ListItem *first;
};

struct TableCell {
  InlineNode *content;
  TableCell *next;
};

struct TableRow {
  TableCell *first;
  TableRow *next;
};

struct Table {
  s32 columns;
  TableRow *first;
};

struct StyleProp {
  const char *key;
  const char *value;
  StyleProp *next;
};

struct SetRule {
  const char *target;
  StyleProp *props;
};

struct ShowRule {
  const char *target;
  const char *class_name;
};

struct LayoutBlock {
  const char *kind;
  s32 columns;
  InlineNode *content;
};

using BlockItem = std::variant<Heading, Paragraph, List, Table, SetRule, ShowRule, LayoutBlock, FuncCall>;

struct BlockNode {
  BlockItem item;
  BlockNode *next;
};

struct Document {
  BlockNode *first;
};

} // namespace ir
