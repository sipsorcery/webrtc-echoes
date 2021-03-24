//-----------------------------------------------------------------------------
// Filename: KurentoModel.cs
//
// Description: Object model to allow basic WebRTC test with the Kurento media
// server https://www.kurento.org/.
//
// Author(s):
// Aaron Clauson (aaron@sipsorcery.com)
//
// History:
// 16 Mar 2021	Aaron Clauson	Created, Dublin, Ireland.
//
// License: 
// BSD 3-Clause "New" or "Revised" License, see included LICENSE.md file.
//-----------------------------------------------------------------------------

namespace KurentoEchoClient
{
    public enum KurentoMethodsEnum
    {
        create,
        invoke,
        ping,
        release,
        subscribe,
        unsubscribe,
    }

    public class KurentoResult
    {
        public string value { get; set; }
        public string sessionId { get; set; }
    }

    public class KurentoEvent
    {
        public KurentoEventData data { get; set; }
        public string @object { get; set;}
        public string type { get; set; }
    }

    public class KurentoEventData
    {
        public string source { get; set; }
        public string[] tags { get; set; }
        public string timestamp { get; set; }
        public string timestampMillis { get; set; }
        public string type { get; set; }
    }
}
