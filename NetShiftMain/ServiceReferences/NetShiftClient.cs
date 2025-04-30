using System;
using System.IO;
using System.IO.Pipes;
using System.Text;
using NetShift.Models; // Updated to use NetShift.Models

namespace NetShift
{
    public class NetShiftClient : IIPChangerService
    {
        private const string PipeName = "NetShiftService";
        private readonly string _logPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "NetShift", "client.log");

        private void LogMessage(string message)
        {
            try
            {
                string logEntry = $"[{DateTime.Now:yyyy-MM-dd HH:mm:ss}] {message}";
                string? directoryPath = Path.GetDirectoryName(_logPath);
                if (directoryPath != null)
                {
                    Directory.CreateDirectory(directoryPath);
                }
                File.AppendAllText(_logPath, logEntry + Environment.NewLine);
            }
            catch
            {
                // Ignore logging errors to avoid impacting functionality
            }
        }

        public void SetStaticIP(Preset preset)
        {
            if (preset == null)
                throw new ArgumentNullException(nameof(preset));

            string message = $"SetStaticIP|{preset.Name}|{preset.IpAddress}|{preset.SubnetMask}|{preset.Gateway ?? ""}|{preset.Dns ?? ""}";
            LogMessage($"Sending SetStaticIP request: {message}");

            try
            {
                using (var pipeClient = new NamedPipeClientStream(".", PipeName, PipeDirection.Out))
                {
                    pipeClient.Connect(5000);
                    byte[] buffer = Encoding.Unicode.GetBytes(message);
                    pipeClient.Write(buffer, 0, buffer.Length);
                    LogMessage("SetStaticIP request sent successfully");
                }
            }
            catch (TimeoutException)
            {
                LogMessage("Failed to connect to the NetShift service: Timeout");
                throw new InvalidOperationException("Failed to connect to the NetShift service. Ensure the service is installed and running.");
            }
            catch (Exception ex)
            {
                LogMessage($"Failed to set static IP: {ex.Message}");
                throw new InvalidOperationException($"Failed to set static IP: {ex.Message}", ex);
            }
        }

        public void ResetToDhcp(string adapterName)
        {
            if (string.IsNullOrEmpty(adapterName))
                throw new ArgumentException("Adapter name cannot be null or empty.", nameof(adapterName));

            string message = $"ResetToDhcp|{adapterName}";
            LogMessage($"Sending ResetToDhcp request: {message}");

            try
            {
                using (var pipeClient = new NamedPipeClientStream(".", PipeName, PipeDirection.Out))
                {
                    pipeClient.Connect(5000);
                    byte[] buffer = Encoding.Unicode.GetBytes(message);
                    pipeClient.Write(buffer, 0, buffer.Length);
                    LogMessage("ResetToDhcp request sent successfully");
                }
            }
            catch (TimeoutException)
            {
                LogMessage("Failed to connect to the NetShift service: Timeout");
                throw new InvalidOperationException("Failed to connect to the NetShift service. Ensure the service is installed and running.");
            }
            catch (Exception ex)
            {
                LogMessage($"Failed to reset to DHCP: {ex.Message}");
                throw new InvalidOperationException($"Failed to reset to DHCP: {ex.Message}", ex);
            }
        }
    }
}