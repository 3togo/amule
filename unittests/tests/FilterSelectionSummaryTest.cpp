//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
//

#include "FilterSelectionSummary.h"
#include <muleunit/test.h>

using namespace muleunit;
using namespace FilterSelectionSummary;

DECLARE_SIMPLE(FilterSelectionSummary)

TEST(FilterSelectionSummary, DisabledMasterIgnoresSavedRules)
{
	Settings settings;
	settings.allMessages = settings.nonFriends = settings.messageWords = true;
	settings.messageTerms = "saved phrase";
	const auto summary = Describe(settings);
	ASSERT_TRUE(summary.Contains("message filter is off"));
	ASSERT_TRUE(!summary.Contains("will be discarded"));
	ASSERT_TRUE(!summary.Contains("saved phrase"));
	ASSERT_EQUALS(wxString("saved phrase"), settings.messageTerms);
}

TEST(FilterSelectionSummary, AllMessagesOverridesSelectiveRules)
{
	Settings settings;
	settings.messages = settings.allMessages = settings.nonFriends = settings.messageWords = true;
	settings.messageTerms = "unused";
	settings.advancedSpam = settings.captcha = true;
	const auto summary = Describe(settings);
	ASSERT_TRUE(summary.Contains("All incoming messages outside current chats"));
	ASSERT_TRUE(!summary.Contains("friend list"));
	ASSERT_TRUE(!summary.Contains("unused"));
	ASSERT_TRUE(!summary.Contains("CAPTCHA"));
}

TEST(FilterSelectionSummary, EnabledMasterAloneAndEmptyTermsExplainNoEffect)
{
	Settings settings;
	settings.messages = true;
	ASSERT_TRUE(Describe(settings).Contains("select at least one option"));
	settings.messageWords = true;
	settings.messageTerms = " , ,  ";
	ASSERT_TRUE(Describe(settings).Contains("word list is empty"));
}

TEST(FilterSelectionSummary, SelectiveRulesUseParsedTermsAndExemptCurrentChats)
{
	Settings settings;
	settings.messages = settings.nonFriends = settings.unknownClients = settings.messageWords = true;
	settings.messageTerms = " Dog , , CAT , hot dog ";
	const auto summary = Describe(settings);
	ASSERT_TRUE(summary.Contains("friend list"));
	ASSERT_TRUE(summary.Contains("unknown clients"));
	ASSERT_TRUE(summary.Contains("ignoring case): Dog, CAT, hot dog."));
	ASSERT_TRUE(summary.Contains("Current chats are exempt"));
}

TEST(FilterSelectionSummary, OnlyExactMessageStarBlocksEverything)
{
	Settings settings;
	settings.messages = settings.messageWords = true;
	settings.messageTerms = "*";
	ASSERT_TRUE(Describe(settings).Contains("All incoming messages"));
	for (const auto &text : { " * ", "*,dog", "*," }) {
		settings.messageTerms = text;
		const auto summary = Describe(settings);
		ASSERT_TRUE(!summary.Contains("All incoming messages"));
		ASSERT_TRUE(summary.Contains("ignoring case): *"));
	}
}

TEST(FilterSelectionSummary, CommentTermsAreIndependentAndStarIsLiteral)
{
	Settings settings;
	settings.messages = settings.allMessages = settings.comments = true;
	settings.commentTerms = " * , Dog , , ";
	ASSERT_TRUE(Describe(settings).Contains("literal terms will be hidden (ignoring case): *, Dog."));
	settings.commentTerms = " , ";
	ASSERT_TRUE(Describe(settings).Contains("No file comments will be hidden"));
	settings.comments = false;
	ASSERT_TRUE(Describe(settings).Contains("File comment filtering is off"));
}

TEST(FilterSelectionSummary, AdvancedSpamIsIndependentButCaptchaNeedsItAndCoreSupport)
{
	Settings settings;
	settings.captcha = true;
	ASSERT_TRUE(!Describe(settings).Contains("CAPTCHA"));
	settings.advancedSpam = true;
	const auto summary = Describe(settings);
	ASSERT_TRUE(summary.Contains("message filter is off"));
	ASSERT_TRUE(summary.Contains("Advanced spam filtering"));
	ASSERT_TRUE(summary.Contains("require a CAPTCHA"));
	settings.advancedSpamSupported = false;
	ASSERT_TRUE(!Describe(settings).Contains("Advanced spam filtering"));
	ASSERT_TRUE(!Describe(settings).Contains("CAPTCHA"));
}

TEST(FilterSelectionSummary, ListsPreserveLiteralSpacesAndBackslashes)
{
	Settings settings;
	settings.comments = settings.logMessages = true;
	settings.commentTerms = "hot dog,C:\\spam";
	const auto summary = Describe(settings);
	ASSERT_TRUE(summary.Contains("hot dog, C:\\spam."));
	ASSERT_TRUE(summary.Contains("message text will also be written to the log"));
}
