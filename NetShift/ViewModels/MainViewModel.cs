using System;
using System.Collections.ObjectModel;
using System.Management;
using System.Windows;
using System.Windows.Input;
using System.ServiceModel;
using NetShift.Models;
using NetShift.Views;
using NetShift.ServiceReferences;
using System.Text.Json;

namespace NetShift.ViewModels
{
    public class MainViewModel : NotifyPropertyChangedBase
    {
        private readonly string _presetsFilePath = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "NetShift", "presets.json");

        public ObservableCollection<string> NetworkAdapters { get; } = new ObservableCollection<string>();
        public ObservableCollection<Preset> Presets { get; } = new ObservableCollection<Preset>();

        private string? _selectedAdapter;
        public string? SelectedAdapter
        {
            get => _selectedAdapter;
            set
            {
                _selectedAdapter = value;
                OnPropertyChanged();
                ApplyCommand.RaiseCanExecuteChanged();
                ResetToDhcpCommand.RaiseCanExecuteChanged();
            }
        }

        private Preset? _selectedPreset;
        public Preset? SelectedPreset
        {
            get => _selectedPreset;
            set
            {
                _selectedPreset = value;
                UpdateFieldsFromPreset();
                OnPropertyChanged();
                DeletePresetCommand.RaiseCanExecuteChanged();
            }
        }

        private string? _ipAddress;
        public string? IpAddress
        {
            get => _ipAddress;
            set
            {
                _ipAddress = value;
                OnPropertyChanged();
                ApplyCommand.RaiseCanExecuteChanged();
                SavePresetCommand.RaiseCanExecuteChanged();
            }
        }

        private string? _subnetMask;
        public string? SubnetMask
        {
            get => _subnetMask;
            set
            {
                _subnetMask = value;
                OnPropertyChanged();
                ApplyCommand.RaiseCanExecuteChanged();
                SavePresetCommand.RaiseCanExecuteChanged();
            }
        }

        private string? _gateway;
        public string? Gateway
        {
            get => _gateway;
            set
            {
                _gateway = value;
                OnPropertyChanged();
                ApplyCommand.RaiseCanExecuteChanged();
            }
        }

        private string? _dns;
        public string? Dns
        {
            get => _dns;
            set
            {
                _dns = value;
                OnPropertyChanged();
                ApplyCommand.RaiseCanExecuteChanged();
            }
        }

        public RelayCommand ApplyCommand { get; }
        public RelayCommand SavePresetCommand { get; }
        public RelayCommand DeletePresetCommand { get; }
        public RelayCommand ResetToDhcpCommand { get; }

        public MainViewModel()
        {
            ApplyCommand = new RelayCommand(
                Apply,
                _ => !string.IsNullOrWhiteSpace(SelectedAdapter) && !string.IsNullOrWhiteSpace(IpAddress) && !string.IsNullOrWhiteSpace(SubnetMask)
            );
            SavePresetCommand = new RelayCommand(
                SavePreset,
                _ => !string.IsNullOrWhiteSpace(IpAddress) && !string.IsNullOrWhiteSpace(SubnetMask)
            );
            DeletePresetCommand = new RelayCommand(
                DeletePreset,
                _ => SelectedPreset != null
            );
            ResetToDhcpCommand = new RelayCommand(
                ResetToDhcp,
                _ => !string.IsNullOrWhiteSpace(SelectedAdapter)
            );
            Presets = new ObservableCollection<Preset>();
            // Add a "None" preset
            Presets.Add(new Preset { Name = "None" });
            // Load existing presets (e.g., from a file or database)
            LoadPresets();
            // Default to "None"
            SelectedPreset = Presets[0];
            LoadNetworkAdapters();
        }

        private void LoadNetworkAdapters()
        {
            NetworkAdapters.Clear();
            ManagementObjectSearcher searcher = new ManagementObjectSearcher("SELECT * FROM Win32_NetworkAdapter WHERE NetConnectionID IS NOT NULL AND PhysicalAdapter = true");
            foreach (ManagementObject obj in searcher.Get())
            {
                string? connectionId = obj["NetConnectionID"]?.ToString();
                if (connectionId != null && !connectionId.Contains("Bluetooth Network Connection"))
                {
                    NetworkAdapters.Add(connectionId);
                }
            }

            // Default to "Ethernet" if it exists, otherwise the first NIC
            if (NetworkAdapters.Contains("Ethernet"))
            {
                SelectedAdapter = "Ethernet";
            }
            else if (NetworkAdapters.Count > 0)
            {
                SelectedAdapter = NetworkAdapters[0];
            }
            else
            {
                SelectedAdapter = null;
            }
        }

        private void LoadPresets()
        {
            try
            {
                if (System.IO.File.Exists(_presetsFilePath))
                {
                    string json = System.IO.File.ReadAllText(_presetsFilePath);
                    var presets = JsonSerializer.Deserialize<ObservableCollection<Preset>>(json);
                    if (presets != null)
                    {
                        foreach (var preset in presets)
                        {
                            if (preset.Name != "None") // Ensure "None" isn't duplicated
                            {
                                Presets.Add(preset);
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to load presets: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void SavePresets()
        {
            try
            {
                string? directoryPath = System.IO.Path.GetDirectoryName(_presetsFilePath);
                if (directoryPath != null)
                {
                    System.IO.Directory.CreateDirectory(directoryPath);
                }
                string json = JsonSerializer.Serialize(Presets);
                System.IO.File.WriteAllText(_presetsFilePath, json);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to save presets: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void UpdateFieldsFromPreset()
        {
            if (_selectedPreset == null || _selectedPreset.Name == "None")
            {
                IpAddress = string.Empty;
                SubnetMask = string.Empty;
                Gateway = string.Empty;
                Dns = string.Empty;
            }
            else
            {
                IpAddress = _selectedPreset.IpAddress;
                SubnetMask = _selectedPreset.SubnetMask;
                Gateway = _selectedPreset.Gateway;
                Dns = _selectedPreset.Dns;
            }
        }

        private void Apply(object? parameter)
        {
            // Validate the selected adapter
            if (string.IsNullOrEmpty(SelectedAdapter))
            {
                throw new InvalidOperationException("No network adapter selected. Please select an adapter from the dropdown.");
            }

            try
            {
                var preset = new Preset
                {
                    Name = SelectedAdapter!,
                    IpAddress = IpAddress!,
                    SubnetMask = SubnetMask!,
                    Gateway = Gateway ?? string.Empty,
                    Dns = Dns ?? string.Empty
                };

                using (var channelFactory = new ChannelFactory<IIPChangerService>(new NetNamedPipeBinding(), new EndpointAddress("net.pipe://localhost/IPChangerService")))
                {
                    IIPChangerService? service = null;
                    try
                    {
                        channelFactory.Open();
                        service = channelFactory.CreateChannel();
                        service.SetStaticIP(preset);
                        ((IClientChannel)service).Close();
                        channelFactory.Close();
                        MessageBox.Show("IP settings updated successfully!", "Success", MessageBoxButton.OK, MessageBoxImage.Information);
                    }
                    catch (Exception ex)
                    {
                        // Ensure the channel is aborted if it faults
                        if (service != null)
                        {
                            ((IClientChannel)service).Abort();
                        }
                        channelFactory.Abort();
                        MessageBox.Show($"Failed to update IP settings: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to update IP settings: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void ResetToDhcp(object? parameter)
        {
            try
            {
                if (string.IsNullOrEmpty(SelectedAdapter))
                {
                    throw new InvalidOperationException("No network adapter selected. Please select an adapter from the dropdown.");
                }

                using (var channelFactory = new ChannelFactory<IIPChangerService>(new NetNamedPipeBinding(), new EndpointAddress("net.pipe://localhost/IPChangerService")))
                {
                    var service = channelFactory.CreateChannel();
                    service.ResetToDhcp(SelectedAdapter);
                }

                // Clear the UI fields
                IpAddress = null;
                SubnetMask = null;
                Gateway = null;
                Dns = null;

                MessageBox.Show("NIC reset to DHCP successfully!", "Success", MessageBoxButton.OK, MessageBoxImage.Information);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to reset NIC to DHCP: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void SavePreset(object? parameter)
        {
            var dialog = new PresetNameDialog
            {
                Owner = Application.Current.MainWindow
            };
            var dialogViewModel = new PresetNameDialogViewModel();
            dialog.DataContext = dialogViewModel;

            if (dialog.ShowDialog() == true && !string.IsNullOrWhiteSpace(dialogViewModel.PresetName))
            {
                var preset = new Preset
                {
                    Name = dialogViewModel.PresetName!,
                    IpAddress = IpAddress ?? string.Empty,
                    SubnetMask = SubnetMask ?? string.Empty,
                    Gateway = Gateway ?? string.Empty,
                    Dns = Dns ?? string.Empty
                };
                Presets.Add(preset);
                SavePresets();
                SelectedPreset = preset;
            }
        }

        private void DeletePreset(object? parameter)
        {
            if (SelectedPreset != null && MessageBox.Show($"Delete preset '{SelectedPreset!.Name}'?", "Confirm", MessageBoxButton.YesNo, MessageBoxImage.Question) == MessageBoxResult.Yes)
            {
                Presets.Remove(SelectedPreset);
                SavePresets();
                SelectedPreset = null;
            }
        }
    }

    public abstract class NotifyPropertyChangedBase : System.ComponentModel.INotifyPropertyChanged
    {
        public event System.ComponentModel.PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([System.Runtime.CompilerServices.CallerMemberName] string? propertyName = null)
        {
            PropertyChanged?.Invoke(this, new System.ComponentModel.PropertyChangedEventArgs(propertyName));
        }
    }

    public class RelayCommand : ICommand
    {
        private readonly Action<object?> _execute;
        private readonly Func<object?, bool>? _canExecute;

        public RelayCommand(Action<object?> execute, Func<object?, bool>? canExecute = null)
        {
            _execute = execute ?? throw new ArgumentNullException(nameof(execute));
            _canExecute = canExecute;
        }

        public event EventHandler? CanExecuteChanged;

        public bool CanExecute(object? parameter)
        {
            return _canExecute == null || _canExecute(parameter);
        }

        public void Execute(object? parameter)
        {
            if (CanExecute(parameter))
            {
                _execute(parameter);
            }
        }

        public void RaiseCanExecuteChanged()
        {
            CanExecuteChanged?.Invoke(this, EventArgs.Empty);
        }
    }
}