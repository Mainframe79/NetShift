using System.Runtime.Serialization;

namespace NetShift.Models
{
    [DataContract(Namespace = "http://NetShift.Models")]
    public class Preset
    {
        [DataMember]
        public string Name { get; set; }

        [DataMember]
        public string IpAddress { get; set; }

        [DataMember]
        public string SubnetMask { get; set; }

        [DataMember]
        public string Gateway { get; set; }

        [DataMember]
        public string Dns { get; set; }
    }
}