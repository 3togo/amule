// This file is part of aMule, licensed under the GNU GPL version 2 or later.
#include <muleunit/test.h>
#include <ec/cpp/ECUserEvents.h>

using namespace muleunit;
DECLARE_SIMPLE(ECUserEvents)

namespace
{
CUserEventData Completion()
{
	return { "DownloadCompleted",
		{ { "FILE", wxString::FromUTF8("/incoming/測試 %NAME.txt") },
			{ "NAME", wxString::FromUTF8("測試 %NAME.txt") },
			{ "HASH", "0123456789abcdef0123456789abcdef" },
			{ "SIZE", "42" },
			{ "DLACTIVETIME", "0:01" } } };
}
} // namespace

TEST(ECUserEvents, CompletionCarriesASnapshotAndAllCommandVariables)
{
	CUserEventStream stream;
	CUserEventData original = Completion();
	stream.Add(original);
	original.variables[0].second = "changed";
	CUserEventCursor cursor;
	const auto received = cursor.Read(stream.Read(0));
	ASSERT_EQUALS(1u, received.size());
	ASSERT_EQUALS(wxString("DownloadCompleted"), received[0].key);
	ASSERT_EQUALS(wxString::FromUTF8("/incoming/測試 %NAME.txt"), received[0].variables[0].second);
	ASSERT_EQUALS(wxString("42"), received[0].variables[3].second);
	ASSERT_EQUALS(wxString("0:01"), received[0].variables[4].second);
	ASSERT_EQUALS(uint64(1), cursor.Last());
}

TEST(ECUserEvents, CommandValuesAreNotExpandedAgain)
{
	ASSERT_EQUALS(wxString::FromUTF8("/incoming/測試 %NAME.txt|測試 %NAME.txt|42|%UNKNOWN"),
		ExpandUserEventCommand("%FILE|%NAME|%SIZE|%UNKNOWN", Completion()));
}

TEST(ECUserEvents, DuplicateRepliesDoNotRepeatExecution)
{
	CUserEventStream stream;
	stream.Add(Completion());
	const CECPacket reply = stream.Read(0);
	CUserEventCursor cursor;
	ASSERT_EQUALS(1u, cursor.Read(reply).size());
	ASSERT_EQUALS(0u, cursor.Read(reply).size());
}

TEST(ECUserEvents, GUIsHaveIndependentCursors)
{
	CUserEventStream stream;
	stream.Add(Completion());
	CUserEventCursor first;
	CUserEventCursor second;
	ASSERT_EQUALS(1u, first.Read(stream.Read(first.Last())).size());
	ASSERT_EQUALS(1u, second.Read(stream.Read(second.Last())).size());
	stream.Add({ "ErrorOnCompletion", { { "FILE", "/incoming/failed" } } });
	ASSERT_EQUALS(1u, first.Read(stream.Read(first.Last())).size());
	ASSERT_EQUALS(1u, second.Read(stream.Read(second.Last())).size());
}

TEST(ECUserEvents, LoginAndReconnectSkipEarlierEvents)
{
	CUserEventStream stream;
	stream.Add(Completion());
	CUserEventCursor cursor;
	cursor.Reset(stream.Latest());
	ASSERT_EQUALS(0u, cursor.Read(stream.Read(cursor.Last())).size());
	stream.Add({ "OutOfDiskSpace", { { "PARTITION", "/tmp/daemon" } } });
	const auto received = cursor.Read(stream.Read(cursor.Last()));
	ASSERT_EQUALS(1u, received.size());
	ASSERT_EQUALS(wxString("/tmp/daemon"), received[0].variables[0].second);
	stream.Add(Completion());
	cursor.Reset(stream.Latest());
	ASSERT_EQUALS(0u, cursor.Read(stream.Read(0)).size());
}

TEST(ECUserEvents, HistoryIsBoundedAndPollingCanResume)
{
	CUserEventStream stream;
	for (size_t i = 0; i < CUserEventStream::Capacity + 10; ++i) {
		stream.Add(Completion());
	}
	CUserEventCursor cursor;
	ASSERT_EQUALS(CUserEventStream::Capacity, cursor.Read(stream.Read(0)).size());
	ASSERT_EQUALS(uint64(CUserEventStream::Capacity + 10), cursor.Last());
	stream.Add(Completion());
	ASSERT_EQUALS(1u, cursor.Read(stream.Read(cursor.Last())).size());
}

TEST(ECUserEvents, ChatAndIncompleteEventsAreNotRelayed)
{
	CUserEventStream stream;
	stream.Add({ "NewChatSession", { { "SENDER", "peer" } } });
	stream.Add({ "DownloadCompleted", { { "FILE", "/incoming/incomplete" } } });
	ASSERT_EQUALS(uint64(0), stream.Latest());
}

TEST(ECUserEvents, MalformedPayloadCannotSupplyCommandVariables)
{
	CECPacket reply(EC_OP_USER_EVENTS);
	reply.AddTag(CECTag(EC_TAG_USER_EVENT_CURSOR, uint64(1)));
	CECTag event(EC_TAG_USER_EVENT, uint64(1));
	event.AddTag(CECTag(EC_TAG_USER_EVENT_KEY, wxString("ErrorOnCompletion")));
	CECTag variable(EC_TAG_USER_EVENT_VARIABLE, wxString("COMMAND"));
	variable.AddTag(CECTag(EC_TAG_USER_EVENT_VALUE, wxString("touch /tmp/remote")));
	event.AddTag(variable);
	reply.AddTag(event);
	CUserEventCursor cursor;
	ASSERT_EQUALS(0u, cursor.Read(reply).size());
	ASSERT_EQUALS(uint64(1), cursor.Last());
}

TEST(ECUserEvents, FailedOrStaleRepliesDoNotRewindCursor)
{
	CUserEventCursor cursor;
	cursor.Reset(5);
	ASSERT_EQUALS(0u, cursor.Read(CECPacket(EC_OP_FAILED)).size());
	CECPacket stale(EC_OP_USER_EVENTS);
	stale.AddTag(CECTag(EC_TAG_USER_EVENT_CURSOR, uint64(1)));
	ASSERT_EQUALS(0u, cursor.Read(stale).size());
	ASSERT_EQUALS(uint64(5), cursor.Last());
}

TEST(ECUserEvents, DisabledCommandsAndLegacyDaemonsDoNotPoll)
{
	CUserEventSubscription subscription;
	CECPacket request(EC_OP_NOOP);
	ASSERT_FALSE(subscription.BeginPoll(0, true, request));
	ASSERT_FALSE(subscription.BeginPoll(1, false, request));
}

TEST(ECUserEvents, EnablingSkipsHistoryBeforeNormalPolling)
{
	CUserEventStream stream;
	stream.Add(Completion());
	CUserEventSubscription subscription;
	CECPacket request(EC_OP_NOOP);
	ASSERT_TRUE(subscription.BeginPoll(1, true, request));
	ASSERT_TRUE(request.GetTagByName(EC_TAG_USER_EVENT_CURSOR) == nullptr);
	ASSERT_EQUALS(0u, subscription.Read(stream.Read(0, true), 1).size());
	stream.Add(Completion());
	ASSERT_TRUE(subscription.BeginPoll(1, true, request));
	ASSERT_EQUALS(uint64(1), request.GetTagByName(EC_TAG_USER_EVENT_CURSOR)->GetInt());
	ASSERT_FALSE(subscription.BeginPoll(1, true, request));
	ASSERT_EQUALS(1u, subscription.Read(stream.Read(1), 1).size());
}

TEST(ECUserEvents, ChangedCommandsDiscardInFlightEventsAndResynchronize)
{
	CUserEventStream stream;
	CUserEventSubscription subscription;
	CECPacket request(EC_OP_NOOP);
	ASSERT_TRUE(subscription.BeginPoll(1, true, request));
	subscription.Read(stream.Read(0, true), 1);
	stream.Add(Completion());
	ASSERT_TRUE(subscription.BeginPoll(1, true, request));
	ASSERT_EQUALS(0u, subscription.Read(stream.Read(0), 2).size());
	ASSERT_TRUE(subscription.BeginPoll(2, true, request));
	ASSERT_TRUE(request.GetTagByName(EC_TAG_USER_EVENT_CURSOR) == nullptr);
	ASSERT_EQUALS(0u, subscription.Read(stream.Read(0, true), 2).size());
}

TEST(ECUserEvents, ReenablingAndPreferenceResetCannotReplayQueuedEvents)
{
	CUserEventStream stream;
	CUserEventSubscription subscription;
	CECPacket request(EC_OP_NOOP);
	ASSERT_TRUE(subscription.BeginPoll(1, true, request));
	subscription.Read(stream.Read(0, true), 1);
	ASSERT_FALSE(subscription.BeginPoll(0, true, request));
	stream.Add(Completion());
	ASSERT_TRUE(subscription.BeginPoll(1, true, request));
	ASSERT_TRUE(request.GetTagByName(EC_TAG_USER_EVENT_CURSOR) == nullptr);
	subscription.Reset(0);
	ASSERT_FALSE(subscription.BeginPoll(1, true, request));
	ASSERT_EQUALS(0u, subscription.Read(stream.Read(0), 1).size());
	ASSERT_TRUE(subscription.BeginPoll(1, true, request));
	ASSERT_TRUE(request.GetTagByName(EC_TAG_USER_EVENT_CURSOR) == nullptr);
}
