  # Show controller.
  get "api/show/running-config(.:format)", to: "shows#running_config"
  get "api/show/access-list(.:format)", to: "shows#access_list"
  get "api/show/interfaces(.:format)", to: "shows#interfaces"
