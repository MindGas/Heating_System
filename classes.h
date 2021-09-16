class attribute
{
  public:
    uint16_t id;
    uint8_t* value;
    uint8_t val_len;
    uint8_t type;
    attribute(uint16_t a_id, uint8_t* a_value, uint8_t a_val_len, uint8_t a_type)
    {
      id = a_id;
      value =  new uint8_t[a_val_len];
      memcpy(value, a_value, a_val_len);
      //value = a_value;
      val_len = a_val_len;
      type = a_type;
    }
};


class Cluster
{
  private:
    attribute* attributes;
    uint8_t num_attr;

  public:
    uint16_t id;
    Cluster(uint16_t cl_id, attribute* attr, uint8_t num)
    {
      id = cl_id;
      attributes = attr;
      num_attr = num;
    }
    
    attribute* GetAttr(uint16_t attr_id)
    {
      for (uint8_t i = 0; i < num_attr; i++)
      {
        //nss.println(num_attr);
        if (attributes[i].id == attr_id) {
          //attribute res = attributes[i];
          return &attributes[i];
        }
        else {
          //nss.print("T: ");
          //nss.println(attributes[i].id);
        }

      }
      nss.print(F("Attr Not Found: "));
      nss.println(attr_id, HEX);
      return &attribute{0, 0, 0, 0};
    }

};


class LocalMac
{
  private:
    uint8_t addr;
  public:
    void Set(XBeeAddress64 mac) {
      EEPROM.put(addr, mac);
    }

    XBeeAddress64 Get()
    {
      XBeeAddress64 mac;
      EEPROM.get(addr, mac);
      return mac;
    }

    LocalMac(uint8_t mem_loc = 0) {
      addr = mem_loc;
    }

};


class Endpoint
{
  private:
    uint8_t num_in_clusters;
    uint8_t num_out_clusters;

    Cluster* out_clusters;
    uint16_t dev_type;
  public:
    uint8_t id;
    Cluster* in_clusters;
  public:
    Endpoint(uint8_t ep_id = 0, uint16_t type_dev = 0, Cluster* in_cls = {}, Cluster* out_cls = {}, uint8_t num_in_cls = 0, uint8_t num_out_cls = 0)
    {
      id = ep_id;
      dev_type = type_dev;
      num_in_clusters = num_in_cls;
      num_out_clusters = num_out_cls;
      in_clusters = in_cls;
      out_clusters = out_cls;

    }
    Cluster GetCluster(uint16_t cl_id)
    {
      for (uint8_t i = 0; i < num_in_clusters; i++)
      {
        if (cl_id == in_clusters[i].id) {
          return in_clusters[i];
        }
        else
        {
          nss.println(in_clusters[i].id);
        }

      }
      nss.print(F("No Cl "));
      nss.println(cl_id);
    }
    void GetInClusters(uint16_t* in_cl)
    {
      for (uint8_t i = 0; i < num_in_clusters; i++)
      {
        *(in_cl + i) = in_clusters[i].id;
      }
    }
    void GetOutClusters(uint16_t* out_cl)
    {
      for (uint8_t i = 0; i < num_out_clusters; i++)
      {
        *(out_cl + i) = out_clusters[i].id;
      }
    }
    uint8_t GetNumInClusters() {
      return num_in_clusters;
    }
    uint8_t GetNumOutClusters() {
      return num_out_clusters;
    }
    uint16_t GetDevType() {
      return dev_type;
    }
};
