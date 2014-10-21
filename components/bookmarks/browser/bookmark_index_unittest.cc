// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/bookmarks/browser/bookmark_index.h"

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/bookmarks/browser/bookmark_match.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::ASCIIToUTF16;
using base::UTF8ToUTF16;

namespace bookmarks {
namespace {

const char kAboutBlankURL[] = "about:blank";

class BookmarkClientMock : public TestBookmarkClient {
 public:
  BookmarkClientMock(const std::map<GURL, int>& typed_count_map)
      : typed_count_map_(typed_count_map) {}

  bool SupportsTypedCountForNodes() override { return true; }

  void GetTypedCountForNodes(
      const NodeSet& nodes,
      NodeTypedCountPairs* node_typed_count_pairs) override {
    for (NodeSet::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
      const BookmarkNode* node = *it;
      std::map<GURL, int>::const_iterator found =
          typed_count_map_.find(node->url());
      if (found == typed_count_map_.end())
        continue;

      node_typed_count_pairs->push_back(std::make_pair(node, found->second));
    }
  }

 private:
  const std::map<GURL, int> typed_count_map_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkClientMock);
};

class BookmarkIndexTest : public testing::Test {
 public:
  BookmarkIndexTest() : model_(client_.CreateModel()) {}

  typedef std::pair<std::string, std::string> TitleAndURL;

  void AddBookmarks(const char** titles, const char** urls, size_t count) {
    // The pair is (title, url).
    std::vector<TitleAndURL> bookmarks;
    for (size_t i = 0; i < count; ++i) {
      TitleAndURL bookmark(titles[i], urls[i]);
      bookmarks.push_back(bookmark);
    }
    AddBookmarks(bookmarks);
  }

  void AddBookmarks(const std::vector<TitleAndURL>& bookmarks) {
    for (size_t i = 0; i < bookmarks.size(); ++i) {
      model_->AddURL(model_->other_node(), static_cast<int>(i),
                     ASCIIToUTF16(bookmarks[i].first),
                     GURL(bookmarks[i].second));
    }
  }

  void ExpectMatches(const std::string& query,
                     const char** expected_titles,
                     size_t expected_count) {
    std::vector<std::string> title_vector;
    for (size_t i = 0; i < expected_count; ++i)
      title_vector.push_back(expected_titles[i]);
    ExpectMatches(query, title_vector);
  }

  void ExpectMatches(const std::string& query,
                     const std::vector<std::string>& expected_titles) {
    std::vector<BookmarkMatch> matches;
    model_->GetBookmarksMatching(ASCIIToUTF16(query), 1000, &matches);
    ASSERT_EQ(expected_titles.size(), matches.size());
    for (size_t i = 0; i < expected_titles.size(); ++i) {
      bool found = false;
      for (size_t j = 0; j < matches.size(); ++j) {
        if (ASCIIToUTF16(expected_titles[i]) == matches[j].node->GetTitle()) {
          matches.erase(matches.begin() + j);
          found = true;
          break;
        }
      }
      ASSERT_TRUE(found);
    }
  }

  void ExtractMatchPositions(const std::string& string,
                             BookmarkMatch::MatchPositions* matches) {
    std::vector<std::string> match_strings;
    base::SplitString(string, ':', &match_strings);
    for (size_t i = 0; i < match_strings.size(); ++i) {
      std::vector<std::string> chunks;
      base::SplitString(match_strings[i], ',', &chunks);
      ASSERT_EQ(2U, chunks.size());
      matches->push_back(BookmarkMatch::MatchPosition());
      int chunks0, chunks1;
      EXPECT_TRUE(base::StringToInt(chunks[0], &chunks0));
      EXPECT_TRUE(base::StringToInt(chunks[1], &chunks1));
      matches->back().first = chunks0;
      matches->back().second = chunks1;
    }
  }

  void ExpectMatchPositions(
      const BookmarkMatch::MatchPositions& actual_positions,
      const BookmarkMatch::MatchPositions& expected_positions) {
    ASSERT_EQ(expected_positions.size(), actual_positions.size());
    for (size_t i = 0; i < expected_positions.size(); ++i) {
      EXPECT_EQ(expected_positions[i].first, actual_positions[i].first);
      EXPECT_EQ(expected_positions[i].second, actual_positions[i].second);
    }
  }

 protected:
  TestBookmarkClient client_;
  scoped_ptr<BookmarkModel> model_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BookmarkIndexTest);
};

// Various permutations with differing input, queries and output that exercises
// all query paths.
TEST_F(BookmarkIndexTest, GetBookmarksMatching) {
  struct TestData {
    const std::string titles;
    const std::string query;
    const std::string expected;
  } data[] = {
    // Trivial test case of only one term, exact match.
    { "a;b",                        "A",        "a" },

    // Prefix match, one term.
    { "abcd;abc;b",                 "abc",      "abcd;abc" },

    // Prefix match, multiple terms.
    { "abcd cdef;abcd;abcd cdefg",  "abc cde",  "abcd cdef;abcd cdefg"},

    // Exact and prefix match.
    { "ab cdef;abcd;abcd cdefg",    "ab cdef",  "ab cdef"},

    // Exact and prefix match.
    { "ab cdef ghij;ab;cde;cdef;ghi;cdef ab;ghij ab",
      "ab cde ghi",
      "ab cdef ghij"},

    // Title with term multiple times.
    { "ab ab",                      "ab",       "ab ab"},

    // Make sure quotes don't do a prefix match.
    { "think",                      "\"thi\"",  ""},

    // Prefix matches against multiple candidates.
    { "abc1 abc2 abc3 abc4", "abc", "abc1 abc2 abc3 abc4"},
  };
  for (size_t i = 0; i < arraysize(data); ++i) {
    std::vector<std::string> titles;
    base::SplitString(data[i].titles, ';', &titles);
    std::vector<TitleAndURL> bookmarks;
    for (size_t j = 0; j < titles.size(); ++j) {
      TitleAndURL bookmark(titles[j], kAboutBlankURL);
      bookmarks.push_back(bookmark);
    }
    AddBookmarks(bookmarks);

    std::vector<std::string> expected;
    if (!data[i].expected.empty())
      base::SplitString(data[i].expected, ';', &expected);

    ExpectMatches(data[i].query, expected);

    model_ = client_.CreateModel();
  }
}

// Analogous to GetBookmarksMatching, this test tests various permutations
// of title, URL, and input to see if the title/URL matches the input as
// expected.
TEST_F(BookmarkIndexTest, GetBookmarksMatchingWithURLs) {
  struct TestData {
    const std::string query;
    const std::string title;
    const std::string url;
    const bool should_be_retrieved;
  } data[] = {
    // Test single-word inputs.  Include both exact matches and prefix matches.
    { "foo", "Foo",    "http://www.bar.com/",    true  },
    { "foo", "Foodie", "http://www.bar.com/",    true  },
    { "foo", "Bar",    "http://www.foo.com/",    true  },
    { "foo", "Bar",    "http://www.foodie.com/", true  },
    { "foo", "Foo",    "http://www.foo.com/",    true  },
    { "foo", "Bar",    "http://www.bar.com/",    false },
    { "foo", "Bar",    "http://www.bar.com/blah/foo/blah-again/ ",    true  },
    { "foo", "Bar",    "http://www.bar.com/blah/foodie/blah-again/ ", true  },
    { "foo", "Bar",    "http://www.bar.com/blah-foo/blah-again/ ",    true  },
    { "foo", "Bar",    "http://www.bar.com/blah-foodie/blah-again/ ", true  },
    { "foo", "Bar",    "http://www.bar.com/blahafoo/blah-again/ ",    false },

    // Test multi-word inputs.
    { "foo bar", "Foo Bar",      "http://baz.com/",   true  },
    { "foo bar", "Foodie Bar",   "http://baz.com/",   true  },
    { "bar foo", "Foo Bar",      "http://baz.com/",   true  },
    { "bar foo", "Foodie Barly", "http://baz.com/",   true  },
    { "foo bar", "Foo Baz",      "http://baz.com/",   false },
    { "foo bar", "Foo Baz",      "http://bar.com/",   true  },
    { "foo bar", "Foo Baz",      "http://barly.com/", true  },
    { "foo bar", "Foodie Baz",   "http://barly.com/", true  },
    { "bar foo", "Foo Baz",      "http://bar.com/",   true  },
    { "bar foo", "Foo Baz",      "http://barly.com/", true  },
    { "foo bar", "Baz Bar",      "http://blah.com/foo",         true  },
    { "foo bar", "Baz Barly",    "http://blah.com/foodie",      true  },
    { "foo bar", "Baz Bur",      "http://blah.com/foo/bar",     true  },
    { "foo bar", "Baz Bur",      "http://blah.com/food/barly",  true  },
    { "foo bar", "Baz Bur",      "http://bar.com/blah/foo",     true  },
    { "foo bar", "Baz Bur",      "http://barly.com/blah/food",  true  },
    { "foo bar", "Baz Bur",      "http://bar.com/blah/flub",    false },
    { "foo bar", "Baz Bur",      "http://foo.com/blah/flub",    false }
  };

  for (size_t i = 0; i < arraysize(data); ++i) {
    model_ = client_.CreateModel();
    std::vector<TitleAndURL> bookmarks;
    bookmarks.push_back(TitleAndURL(data[i].title, data[i].url));
    AddBookmarks(bookmarks);

    std::vector<std::string> expected;
    if (data[i].should_be_retrieved)
      expected.push_back(data[i].title);

    ExpectMatches(data[i].query, expected);
  }
}

TEST_F(BookmarkIndexTest, Normalization) {
  struct TestData {
    const char* const title;
    const char* const query;
  } data[] = {
    { "fooa\xcc\x88-test", "foo\xc3\xa4-test" },
    { "fooa\xcc\x88-test", "fooa\xcc\x88-test" },
    { "fooa\xcc\x88-test", "foo\xc3\xa4" },
    { "fooa\xcc\x88-test", "fooa\xcc\x88" },
    { "fooa\xcc\x88-test", "foo" },
    { "foo\xc3\xa4-test", "foo\xc3\xa4-test" },
    { "foo\xc3\xa4-test", "fooa\xcc\x88-test" },
    { "foo\xc3\xa4-test", "foo\xc3\xa4" },
    { "foo\xc3\xa4-test", "fooa\xcc\x88" },
    { "foo\xc3\xa4-test", "foo" },
    { "foo", "foo" }
  };

  GURL url(kAboutBlankURL);
  for (size_t i = 0; i < arraysize(data); ++i) {
    model_->AddURL(model_->other_node(), 0, UTF8ToUTF16(data[i].title), url);
    std::vector<BookmarkMatch> matches;
    model_->GetBookmarksMatching(UTF8ToUTF16(data[i].query), 10, &matches);
    EXPECT_EQ(1u, matches.size());
    model_ = client_.CreateModel();
  }
}

// Makes sure match positions are updated appropriately for title matches.
TEST_F(BookmarkIndexTest, MatchPositionsTitles) {
  struct TestData {
    const std::string title;
    const std::string query;
    const std::string expected_title_match_positions;
  } data[] = {
    // Trivial test case of only one term, exact match.
    { "a",                        "A",        "0,1" },
    { "foo bar",                  "bar",      "4,7" },
    { "fooey bark",               "bar foo",  "0,3:6,9" },
    // Non-trivial tests.
    { "foobar foo",               "foobar foo",   "0,6:7,10" },
    { "foobar foo",               "foo foobar",   "0,6:7,10" },
    { "foobar foobar",            "foobar foo",   "0,6:7,13" },
    { "foobar foobar",            "foo foobar",   "0,6:7,13" },
  };
  for (size_t i = 0; i < arraysize(data); ++i) {
    std::vector<TitleAndURL> bookmarks;
    TitleAndURL bookmark(data[i].title, kAboutBlankURL);
    bookmarks.push_back(bookmark);
    AddBookmarks(bookmarks);

    std::vector<BookmarkMatch> matches;
    model_->GetBookmarksMatching(ASCIIToUTF16(data[i].query), 1000, &matches);
    ASSERT_EQ(1U, matches.size());

    BookmarkMatch::MatchPositions expected_title_matches;
    ExtractMatchPositions(data[i].expected_title_match_positions,
                          &expected_title_matches);
    ExpectMatchPositions(matches[0].title_match_positions,
                         expected_title_matches);

    model_ = client_.CreateModel();
  }
}

// Makes sure match positions are updated appropriately for URL matches.
TEST_F(BookmarkIndexTest, MatchPositionsURLs) {
  // The encoded stuff between /wiki/ and the # is 第二次世界大戦
  const std::string ja_wiki_url = "http://ja.wikipedia.org/wiki/%E7%AC%AC%E4"
      "%BA%8C%E6%AC%A1%E4%B8%96%E7%95%8C%E5%A4%A7%E6%88%A6#.E3.83.B4.E3.82.A7"
      ".E3.83.AB.E3.82.B5.E3.82.A4.E3.83.A6.E4.BD.93.E5.88.B6";
  struct TestData {
    const std::string query;
    const std::string url;
    const std::string expected_url_match_positions;
  } data[] = {
    { "foo",        "http://www.foo.com/",    "11,14" },
    { "foo",        "http://www.foodie.com/", "11,14" },
    { "foo",        "http://www.foofoo.com/", "11,14" },
    { "www",        "http://www.foo.com/",    "7,10"  },
    { "foo",        "http://www.foodie.com/blah/foo/fi", "11,14:27,30"      },
    { "foo",        "http://www.blah.com/blah/foo/fi",   "25,28"            },
    { "foo www",    "http://www.foodie.com/blah/foo/fi", "7,10:11,14:27,30" },
    { "www foo",    "http://www.foodie.com/blah/foo/fi", "7,10:11,14:27,30" },
    { "www bla",    "http://www.foodie.com/blah/foo/fi", "7,10:22,25"       },
    { "http",       "http://www.foo.com/",               "0,4"              },
    { "http www",   "http://www.foo.com/",               "0,4:7,10"         },
    { "http foo",   "http://www.foo.com/",               "0,4:11,14"        },
    { "http foo",   "http://www.bar.com/baz/foodie/hi",  "0,4:23,26"        },
    { "第二次",      ja_wiki_url,                         "29,56"            },
    { "ja 第二次",   ja_wiki_url,                         "7,9:29,56"        },
    { "第二次 E3.8", ja_wiki_url,                         "29,56:94,98:103,107:"
                                                         "112,116:121,125:"
                                                         "130,134:139,143"  }
  };

  for (size_t i = 0; i < arraysize(data); ++i) {
    model_ = client_.CreateModel();
    std::vector<TitleAndURL> bookmarks;
    TitleAndURL bookmark("123456", data[i].url);
    bookmarks.push_back(bookmark);
    AddBookmarks(bookmarks);

    std::vector<BookmarkMatch> matches;
    model_->GetBookmarksMatching(UTF8ToUTF16(data[i].query), 1000, &matches);
    ASSERT_EQ(1U, matches.size()) << data[i].url << data[i].query;

    BookmarkMatch::MatchPositions expected_url_matches;
    ExtractMatchPositions(data[i].expected_url_match_positions,
                          &expected_url_matches);
    ExpectMatchPositions(matches[0].url_match_positions, expected_url_matches);
  }
}

// Makes sure index is updated when a node is removed.
TEST_F(BookmarkIndexTest, Remove) {
  const char* titles[] = { "a", "b" };
  const char* urls[] = {kAboutBlankURL, kAboutBlankURL};
  AddBookmarks(titles, urls, arraysize(titles));

  // Remove the node and make sure we don't get back any results.
  model_->Remove(model_->other_node(), 0);
  ExpectMatches("A", NULL, 0U);
}

// Makes sure index is updated when a node's title is changed.
TEST_F(BookmarkIndexTest, ChangeTitle) {
  const char* titles[] = { "a", "b" };
  const char* urls[] = {kAboutBlankURL, kAboutBlankURL};
  AddBookmarks(titles, urls, arraysize(titles));

  // Remove the node and make sure we don't get back any results.
  const char* expected[] = { "blah" };
  model_->SetTitle(model_->other_node()->GetChild(0), ASCIIToUTF16("blah"));
  ExpectMatches("BlAh", expected, arraysize(expected));
}

// Makes sure no more than max queries is returned.
TEST_F(BookmarkIndexTest, HonorMax) {
  const char* titles[] = { "abcd", "abcde" };
  const char* urls[] = {kAboutBlankURL, kAboutBlankURL};
  AddBookmarks(titles, urls, arraysize(titles));

  std::vector<BookmarkMatch> matches;
  model_->GetBookmarksMatching(ASCIIToUTF16("ABc"), 1, &matches);
  EXPECT_EQ(1U, matches.size());
}

// Makes sure if the lower case string of a bookmark title is more characters
// than the upper case string no match positions are returned.
TEST_F(BookmarkIndexTest, EmptyMatchOnMultiwideLowercaseString) {
  const BookmarkNode* n1 = model_->AddURL(model_->other_node(), 0,
                                          base::WideToUTF16(L"\u0130 i"),
                                          GURL("http://www.google.com"));

  std::vector<BookmarkMatch> matches;
  model_->GetBookmarksMatching(ASCIIToUTF16("i"), 100, &matches);
  ASSERT_EQ(1U, matches.size());
  EXPECT_EQ(n1, matches[0].node);
  EXPECT_TRUE(matches[0].title_match_positions.empty());
}

TEST_F(BookmarkIndexTest, GetResultsSortedByTypedCount) {
  struct TestData {
    const GURL url;
    const char* title;
    const int typed_count;
  } data[] = {
    { GURL("http://www.google.com/"),      "Google",           100 },
    { GURL("http://maps.google.com/"),     "Google Maps",       40 },
    { GURL("http://docs.google.com/"),     "Google Docs",       50 },
    { GURL("http://reader.google.com/"),   "Google Reader",     80 },
  };

  std::map<GURL, int> typed_count_map;
  for (size_t i = 0; i < arraysize(data); ++i)
    typed_count_map.insert(std::make_pair(data[i].url, data[i].typed_count));

  BookmarkClientMock client(typed_count_map);
  scoped_ptr<BookmarkModel> model = client.CreateModel();

  for (size_t i = 0; i < arraysize(data); ++i)
    // Populate the BookmarkIndex.
    model->AddURL(
        model->other_node(), i, UTF8ToUTF16(data[i].title), data[i].url);

  // Populate match nodes.
  std::vector<BookmarkMatch> matches;
  model->GetBookmarksMatching(ASCIIToUTF16("google"), 4, &matches);

  // The resulting order should be:
  // 1. Google (google.com) 100
  // 2. Google Reader (google.com/reader) 80
  // 3. Google Docs (docs.google.com) 50
  // 4. Google Maps (maps.google.com) 40
  ASSERT_EQ(4, static_cast<int>(matches.size()));
  EXPECT_EQ(data[0].url, matches[0].node->url());
  EXPECT_EQ(data[3].url, matches[1].node->url());
  EXPECT_EQ(data[2].url, matches[2].node->url());
  EXPECT_EQ(data[1].url, matches[3].node->url());

  matches.clear();
  // Select top two matches.
  model->GetBookmarksMatching(ASCIIToUTF16("google"), 2, &matches);

  ASSERT_EQ(2, static_cast<int>(matches.size()));
  EXPECT_EQ(data[0].url, matches[0].node->url());
  EXPECT_EQ(data[3].url, matches[1].node->url());
}

}  // namespace
}  // namespace bookmarks
