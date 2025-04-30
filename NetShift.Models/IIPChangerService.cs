using System.ServiceModel;

namespace NetShift.Models
{
    [ServiceContract]
    public interface IIPChangerService
    {
        [OperationContract]
        void SetStaticIP(Preset preset);

        [OperationContract]
        void ResetToDhcp(string adapterName);
    }
}