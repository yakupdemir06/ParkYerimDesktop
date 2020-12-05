#include "databasemanager.hpp"

#include <QDebug>

DatabaseManager::DatabaseManager()
{
    QDir().mkpath(m_dbfilepath);
    QFile::copy(m_dbResourcePath,m_dbfile);
    QFile(m_dbfile).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(m_dbfile);
    if(database.open()){
        getColorsFromDB();
        getVehicleTypesFromDB();
    }
}


DatabaseManager::~DatabaseManager()
{
    if(database.isOpen()) database.close();
    CreateDatabaseBackup();
}

bool DatabaseManager::ValidateUserLogin(const QString& username, const QString& password, QString& errormsg, User** currentUser)
{
    if(!isConnected()) return false;
    else{
        QSqlQuery query;
        query.prepare("SELECT * FROM Accounts WHERE Username == :usr");
        query.bindValue(":usr",username);
        // 1
        if(!query.exec()){
            errormsg = "1: " + query.lastError().text();
            return false;
        }
        if(!query.next()){
            errormsg = "Invalid username.";
            return false;
        }else{
            query.clear();
            query.prepare("SELECT Password FROM Accounts WHERE Username == :usr");
            query.bindValue(":usr",username);
            //2
            if(!query.exec()){
                errormsg = "2: " + query.lastError().text();
                return false;
            }
            query.next();
            if(password != query.value(0).toString()){
                errormsg = "Invalid password.";
                return false;
            }else{
                QString firstname;
                QString lastname;
                qint32 accountID;
                qint32 accType;
                QDateTime creationDate;
                QString password;
                query.clear();
                query.prepare("SELECT Person.FirstName, Person.LastName, Accounts.ID, Accounts.AccountType, Accounts.DateCreated, Accounts.Password FROM Person"
                              " LEFT JOIN Accounts ON Person.ID = Accounts.fk_PersonID"
                              " WHERE Accounts.Username = :usr");
                query.bindValue(":usr",username);
                // 3
                if(!query.exec()){
                    errormsg = "3: " + query.lastError().text();
                    return false;
                }
                query.next();
                firstname = query.value(0).toString();
                lastname = query.value(1).toString();
                accountID = query.value(2).toInt();
                accType = query.value(3).toInt();
                creationDate = query.value(4).toDateTime();
                password = query.value(5).toString();
                *currentUser = new User(username,firstname,lastname,accountID,accType,creationDate,password);
                return true;
            }
        }
    }
}

bool DatabaseManager::DeleteUser(const QString& username, QString& errmsg)
{
    if(!database.open()) return false;
    QSqlQuery query;
    query.prepare("select ID from Accounts where Username = :usr");
    query.bindValue(":usr",username);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    if(!query.next()){
        errmsg = "This username is not found in database.";
        return false;
    }else{
        quint32 userID = query.value(0).toUInt();
        if(userID == 0){
            errmsg = "Admin account cannot be deleted, but you may change its default password.";
            return false;
        }
        query.clear();
        query.prepare("delete from Accounts where ID = :id");
        query.bindValue(":id",userID);
        if(!query.exec()){
            errmsg = query.lastError().text();
            return false;
        }
        return true;
    }
}

bool DatabaseManager::CreateUser(const QString& firstname, const QString& lastname, const QString& phone, const QString& username, const QString& password, const qint32& usertype, QString &errmsg)
{
    QSqlQuery query;
    query.prepare("insert into Person (FirstName,LastName) values(:fn,:ln)");
    query.bindValue(":fn",firstname);
    query.bindValue(":ln",lastname);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    query.clear();
    query.exec("SELECT last_insert_rowid()");
    query.next();
    qint32 userID = query.value(0).toInt();
    query.clear();
    query.prepare("insert into Phones (PhoneNumber,fk_PersonID) values (:phone,:userID)");
    query.bindValue(":phone",phone);
    query.bindValue(":userID",userID);
    if(!query.exec()){
        errmsg = query.lastError().text();
        query.prepare("delete from Person where ID = :id");
        query.bindValue(":id",userID);
        query.exec();
        return false;
    }
    query.clear();
    query.prepare("insert into Accounts (Username,Password,AccountType,fk_PersonID) values (:usr,:pw,:type,:pid)");
    query.bindValue(":usr",username);
    query.bindValue(":pw",password);
    query.bindValue(":type",usertype);
    query.bindValue(":pid",userID);
    if(!query.exec()){
        query.prepare("delete from Person where ID = :id");
        query.bindValue(":id",userID);
        query.exec();
        return false;
    }
    return true;
}

bool DatabaseManager::NewVehicleEntry(const QString& plate, QString& errmsg, qint32& vehicleID, const QString& model, const QString& type, const QString& color)
{
    QSqlQuery query;
    // check if vehicle already exists
    query.prepare("select ID from Vehicles where Plate = :plate");
    query.bindValue(":plate",plate);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    if(query.next()){
        vehicleID = query.value(0).toInt();
        return true;
    }else{
        // create new vehicle entry
        query.clear();
        query.prepare("insert into Vehicles(Plate,Model,fk_colorID,fk_vehicleTypeID) values(:pl,:model,:colorid,:typeid)");
        query.bindValue(":pl",plate);
        query.bindValue(":model",model);
        query.bindValue(":colorid",m_colors[color]);
        query.bindValue(":typeid",m_vehicleTypes[type]);
        if(!query.exec()){
            errmsg = query.lastError().text();
            return false;
        }
        query.clear();
        query.exec("SELECT last_insert_rowid()");
        query.next();
        vehicleID = query.value(0).toInt();
        return true;
    }
}

bool DatabaseManager::NewPaymentEntry(const qint32& vehicleID, const qint32& planID, QString& errmsg)
{
    QSqlQuery query;
    query.prepare("insert into Payments(fk_VehicleID,fk_PricingPlanID) values(:id,:planid)");
    query.bindValue(":id",vehicleID);
    query.bindValue(":planid",planID);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::GetBillingResult(const QString& plate, QString& errmsg, qint32& out_paymentID, qint64& out_minutes, qint32& out_vehicleID, QDateTime& out_entryDate, qint32& out_planID)
{
    QSqlQuery query;
    qint32 vehicleID;
    query.prepare("select ID from Vehicles where Plate = :plate");
    query.bindValue(":plate",plate);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    if(!query.next()){
        errmsg = "This license plate could not found in database.";
        return false;
    }
    vehicleID = query.value(0).toInt();
    out_vehicleID = vehicleID;
    QDateTime entryDate;
    query.clear();
    query.prepare("select ID, VehicleEntryDate, fk_PricingPlanID from Payments where fk_VehicleID = :vid and isPaymentComplete = false");
    query.bindValue(":vid",vehicleID);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    if(!query.next()){
        errmsg = "Vehicle with this license plate does not have any unpayed billing.";
        return false;
    }else{
        out_paymentID = query.value(0).toInt();
        entryDate = query.value(1).toDateTime();
        out_planID = query.value(2).toInt();
        out_entryDate = entryDate;
        out_minutes = entryDate.secsTo(QDateTime::currentDateTime())/60;
        return true;
    }
}

bool DatabaseManager::CompletePayment(const qint32& vehicleID, const QDateTime& exitDate, const qint64& minutes, const float& price, QString &errmsg, QString payerName)
{
    qint32 paymentID;
    QSqlQuery query;
    query.prepare("select Payments.ID from Payments left join Vehicles on Payments.fk_VehicleID = Vehicles.ID where Vehicles.ID = :vid and Payments.isPaymentComplete = false");
    query.bindValue(":vid",vehicleID);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    if(!query.next()){
        errmsg = "Could not found any payment info for this license plate.";
        return false;
    }
    QTime parkingTime = QTime(0,0).addSecs(60*int(minutes));
    paymentID = query.value(0).toInt();
    query.clear();
    query.prepare("update Payments set PayerName = :payer, HoursParked = :hrs, Price = :price, PaymentDate = :date, isPaymentComplete = true where ID = :pid");
    query.bindValue(":pid",paymentID);
    query.bindValue(":payer",payerName);
    query.bindValue(":hrs",parkingTime.toString("HH:mm"));
    query.bindValue(":price",price);
    query.bindValue(":date",exitDate.toString("yyyy-MM-dd HH:mm:ss"));
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::GetVehicleInformation(const qint32& vehicleID, QString& errmsg, QString& out_plate, QString& out_color, QString& out_type, QString& out_model)
{
    QSqlQuery query;
    query.prepare("select Plate, fk_colorID, fk_vehicleTypeID, Model from Vehicles where ID = :id");
    query.bindValue(":id",vehicleID);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    if(!query.next()){
        errmsg = "Invalid vehicle ID";
        return false;
    }
    qint32 colorID;
    qint32 typeID;
    out_plate = query.value(0).toString();
    colorID = query.value(1).toInt();
    typeID = query.value(2).toInt();
    out_model = query.value(3).toString();
    query.clear();
    query.prepare("select Color from Colors where ID = :id");
    query.bindValue(":id",colorID);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    query.next();
    out_color = query.value(0).toString();
    query.clear();
    query.prepare("select TypeName from VehicleTypes where ID = :id");
    query.bindValue(":id",typeID);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    query.next();
    out_type = query.value(0).toString();
    return true;
}

bool DatabaseManager::GetPricingPlans(QVector<PricingPlan *> &out_plans, QString &errmsg)
{
    // clearing the list first
    for(PricingPlan* plan : out_plans){
        if(plan) delete plan;
    }
    out_plans.clear();
    QSqlQuery query;
    query.prepare("select ID, PlanName, PricePerHour, m_LessThanTwo, m_TwoThree, m_ThreeFour, m_FourFive, m_FiveSix, m_SixSeven, m_SevenEight, m_EightTen, m_TenTwelve, m_MoreThanTwelve"
                  " from PricingPlans");
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    while(query.next()){
        out_plans.append(new PricingPlan(query.value(0).toInt(),
                                         query.value(1).toString(),
                                         query.value(2).toFloat(),
                                         query.value(3).toFloat(),
                                         query.value(4).toFloat(),
                                         query.value(5).toFloat(),
                                         query.value(6).toFloat(),
                                         query.value(7).toFloat(),
                                         query.value(8).toFloat(),
                                         query.value(9).toFloat(),
                                         query.value(10).toFloat(),
                                         query.value(11).toFloat(),
                                         query.value(12).toFloat()));
    }
    return true;
}

bool DatabaseManager::CreateNewPricingPlan(const QString& name, const float& priceperhour, const float& lessthantwo, const float& twothree, const float& threefour, const float& fourfive, const float& fivesix, const float& sixseven, const float& seveneight, const float& eightten, const float& tentwelve, const float& morethantwelve, QVector<PricingPlan*>& plans, QString& errmsg)
{
    QSqlQuery query;
    query.prepare("insert into PricingPlans (PlanName, PricePerHour, m_LessThanTwo, m_TwoThree, m_ThreeFour, m_FourFive, m_FiveSix, m_SixSeven, m_SevenEight, m_EightTen, m_TenTwelve, m_MoreThanTwelve)"
                  " values (:name, :priceperhour, :lessthantwo, :twothree, :threefour, :fourfive, :fivesix, :sixseven, :seveneight, :eightten, :tentwelve, :morethantwelve)");
    query.bindValue(":name",name);
    query.bindValue(":priceperhour",priceperhour);
    query.bindValue(":lessthantwo",lessthantwo);
    query.bindValue(":twothree",twothree);
    query.bindValue(":threefour",threefour);
    query.bindValue(":fourfive",fourfive);
    query.bindValue(":fivesix",fivesix);
    query.bindValue(":sixseven",sixseven);
    query.bindValue(":seveneight",seveneight);
    query.bindValue(":eightten",eightten);
    query.bindValue(":tentwelve",tentwelve);
    query.bindValue(":morethantwelve",morethantwelve);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    if(!GetPricingPlans(plans,errmsg)) return false;
    return true;
}

bool DatabaseManager::DeletePricingPlan(const qint32& planID, QVector<PricingPlan*>& out_plans,QString &errmsg)
{
    if(planID == 0){
        errmsg = "Default plan cannot be removed.";
        return false;
    }
    QSqlQuery query;
    query.prepare("delete from PricingPlans where ID = :id");
    query.bindValue(":id",planID);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    if(!GetPricingPlans(out_plans,errmsg)) return false;
    return true;
}

bool DatabaseManager::UpdatePricingPlan(const qint32& planID, const float& lessthantwo, const float& twothree, const float& threefour, const float& fourfive, const float& fivesix, const float& sixseven, const float& seveneight, const float& eightten, const float& tentwelve, const float& morethantwelve, QVector<PricingPlan *> &out_plans, QString &errmsg)
{
    QSqlQuery query;
    query.prepare("update PricingPlans set"
                  " m_LessThanTwo = :lessthantwo,"
                  " m_TwoThree = :twothree,"
                  " m_ThreeFour = :threefour,"
                  " m_FourFive = :fourfive,"
                  " m_FiveSix = :fivesix,"
                  " m_SixSeven = :sixseven,"
                  " m_SevenEight = :seveneight,"
                  " m_EightTen = :eightten,"
                  " m_TenTwelve = :tentwelve,"
                  " m_MoreThanTwelve = :morethantwelve"
                  " where ID = :id");
    query.bindValue(":lessthantwo",lessthantwo);
    query.bindValue(":twothree",twothree);
    query.bindValue(":threefour",threefour);
    query.bindValue(":fourfive",fourfive);
    query.bindValue(":fivesix",fivesix);
    query.bindValue(":sixseven",sixseven);
    query.bindValue(":seveneight",seveneight);
    query.bindValue(":eightten",eightten);
    query.bindValue(":tentwelve",tentwelve);
    query.bindValue(":morethantwelve",morethantwelve);
    query.bindValue(":id",planID);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    if(!GetPricingPlans(out_plans,errmsg)) return false;
    return true;
}


bool DatabaseManager::SetQueryModel_Employees(QSqlQueryModel* out_model, QString &errmsg)
{
    if(!out_model){
        errmsg = "QueryModel is not initialized.";
        return false;
    }
    if(!database.isOpen()){
        errmsg = "Database is not connected.";
        return false;
    }
    QSqlQuery query;
    query.prepare("select Person.FirstName, Person.LastName, Phones.PhoneNumber, Accounts.Username, Accounts.DateCreated from Accounts"
                  " left join Person on Person.ID = Accounts.fk_personID"
                  " left join Phones on Person.ID = Phones.fk_PersonID"
                  " where Accounts.AccountType = 1");
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    out_model->setQuery(query);
    return true;
}

bool DatabaseManager::SetQueryModel_Managers(QSqlQueryModel *out_model, QString &errmsg)
{
    if(!out_model){
        errmsg = "QueryModel is not initialized.";
        return false;
    }
    if(!database.isOpen()){
        errmsg = "Database is not connected.";
        return false;
    }
    QSqlQuery query;
    query.prepare("select Person.FirstName, Person.LastName, Phones.PhoneNumber, Accounts.Username, Accounts.DateCreated from Accounts"
                  " left join Person on Person.ID = Accounts.fk_personID"
                  " left join Phones on Person.ID = Phones.fk_PersonID"
                  " where Accounts.AccountType = 2 or Accounts.AccountType = 3");
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    out_model->setQuery(query);
    return true;
}

bool DatabaseManager::SetQUeryModel_Payments(QSqlQueryModel *out_model, QString &errmsg)
{
    if(!out_model){
        errmsg = "QueryModel is not initialized.";
        return false;
    }
    if(!database.isOpen()){
        errmsg = "Database is not connected.";
        return false;
    }
    QSqlQuery query;
    query.prepare("select Payments.ID as 'Fatura ID', Vehicles.Plate as 'Plaka', Payments.VehicleEntryDate as 'Giriş Tarihi', Payments.PaymentDate as 'Çıkış Tarihi', Payments.HoursParked as 'Süre', Payments.Price as 'Fiyat (TL)', PricingPlans.PlanName as 'Tarife'"
                  " from Payments left join Vehicles on Vehicles.ID = Payments.fk_VehicleID"
                  " left join PricingPlans on PricingPlans.ID = Payments.fk_PricingPlanID"
                  " where Payments.isPaymentComplete = true");
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    out_model->setQuery(query);
    return true;
}

bool DatabaseManager::QueryMonthlyIncome(float &out_income, QString &errmsg)
{
    QSqlQuery query;
    if(!query.exec("select PaymentDate, Price from Payments where isPaymentComplete = true")){
        errmsg = query.lastError().text();
        return false;
    }
    QVector<PaymentInfo> payments;
    float totalPrice = 0.0f;
    while(query.next()){
        payments.append(PaymentInfo(query.value(0).toDateTime(),query.value(1).toFloat()));
    }
    for(PaymentInfo p : payments){
        if(p.date.daysTo(QDateTime::currentDateTime()) < 30) totalPrice += p.price;
    }
    out_income = totalPrice;
    return true;
}

bool DatabaseManager::QueryWeeklyIncome(float &out_income, QString &errmsg)
{
    QSqlQuery query;
    if(!query.exec("select PaymentDate, Price from Payments where isPaymentComplete = true")){
        errmsg = query.lastError().text();
        return false;
    }
    QVector<PaymentInfo> payments;
    float totalPrice = 0.0f;
    while(query.next()){
        payments.append(PaymentInfo(query.value(0).toDateTime(),query.value(1).toFloat()));
    }
    for(PaymentInfo p : payments){
        if(p.date.daysTo(QDateTime::currentDateTime()) < 7) totalPrice += p.price;
    }
    out_income = totalPrice;
    return true;
}

bool DatabaseManager::QueryDailyIncome(float &out_income, QString &errmsg)
{
    QSqlQuery query;
    if(!query.exec("select PaymentDate, Price from Payments where isPaymentComplete = true")){
        errmsg = query.lastError().text();
        return false;
    }
    QVector<PaymentInfo> payments;
    float totalPrice = 0.0f;
    while(query.next()){
        payments.append(PaymentInfo(query.value(0).toDateTime(),query.value(1).toFloat()));
    }
    for(PaymentInfo p : payments){
        if(p.date.daysTo(QDateTime::currentDateTime()) < 1) totalPrice += p.price;
    }
    out_income = totalPrice;
    return true;
}

bool DatabaseManager::SetQueryModel_TotalPaymentInfo(const qint32& vehicleID, QSqlQueryModel *out_model, QString &errmsg)
{
    QSqlQuery query;
    query.prepare("select Payments.ID as 'Fatura ID', VehicleEntryDate as 'Giriş Tarihi', PaymentDate as 'Çıkış Tarihi', HoursParked as 'Süre', Price as 'Ücret (TL)', PricingPlans.PlanName as 'Tarife' from Payments"
                  " left join PricingPlans on Payments.fk_PricingPlanID = PricingPlans.ID where fk_VehicleID = :id");
    query.bindValue(":id",vehicleID);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    out_model->setQuery(query);
    return true;
}

bool DatabaseManager::SetQueryModel_ListAllVehiclesInside(QSqlQueryModel* out_model, QString& errmsg)
{
    QSqlQuery query;
    query.prepare("select Vehicles.Plate as 'Plaka', Vehicles.Model as 'Marka', Payments.ID as 'Fatura ID', VehicleEntryDate as 'Giriş Saati', PricingPlans.PlanName as 'Tarife' from Payments"
                  " left join PricingPlans on Payments.fk_PricingPlanID = PricingPlans.ID"
                  " left join Vehicles on Payments.fk_VehicleID = Vehicles.ID where Payments.isPaymentComplete = 0");
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    out_model->setQuery(query);
    return true;
}

bool DatabaseManager::GetVehicleInformationByPlate(const QString& plate, qint32 &out_vehicleID, QString &out_color, QString &out_type, QString &out_model, QString& errmsg)
{
    QSqlQuery query;
    query.prepare("select Vehicles.ID, Colors.Color, VehicleTypes.TypeName, Vehicles.Model from Vehicles"
                  " left join Colors on Vehicles.fk_colorID = Colors.ID"
                  " left join VehicleTypes on Vehicles.fk_vehicleTypeID = VehicleTypes.ID"
                  " where Vehicles.Plate = :plate");
    query.bindValue(":plate",plate);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    if(!query.next()){
        errmsg = "Bu plakaya ait bir araç kaydı bulunamadı.";
        return false;
    }
    out_vehicleID = query.value(0).toInt();
    out_color = query.value(1).toString();
    out_type = query.value(2).toString();
    out_model = query.value(3).toString();
    return true;
}

bool DatabaseManager::ChangePassword(const qint32& accountID, const QString& oldPassword, const QString& newPassword, QString &errmsg)
{
    if(oldPassword == newPassword){
        errmsg = "Please enter different password than old one.";
        return false;
    }
    QSqlQuery query;
    query.prepare("select Password from Accounts where ID = :id");
    query.bindValue(":id",accountID);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    if(!query.next()){
        errmsg = "Invalid User ID";
        return false;
    }
    if(oldPassword != query.value(0).toString()){
        errmsg = "Invalid password.";
        return false;
    }
    // apply new password
    query.clear();
    query.prepare("update Accounts set Password = :pw where ID = :id");
    query.bindValue(":pw",newPassword);
    query.bindValue(":id",accountID);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    return true;
}

qint32 DatabaseManager::QueryRemainingSpots(QString& errmsg)
{
    QSqlQuery query;
    query.prepare("select RemainingSpots from OtoparkInfo where ID = 0");
    if(!query.exec()){
        errmsg = query.lastError().text();
        return -1;
    }if(!query.next()){
        errmsg = "Invalid Parking Lot ID";
        return -1;
    }
    return query.value(0).toInt();
}


bool DatabaseManager::SetRemainingSpotCount(const qint32& value, QString& out_errmsg)
{
    QSqlQuery query;
    query.prepare("update OtoparkInfo set RemainingSpots = :val where ID = 0");
    query.bindValue(":val",value);
    if(!query.exec()){
        out_errmsg = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::IncreaseRemainingSpot()
{
    QSqlQuery query;
    if(!query.exec("update OtoparkInfo set RemainingSpots = RemainingSpots+1 where ID = 0")) return false;
    else{
        emit sig_RemainingSpotIncreased();
        return true;
    }
}

bool DatabaseManager::DecreaseRemainingSpot()
{
    QSqlQuery query;
    if(!query.exec("update OtoparkInfo set RemainingSpots = RemainingSpots-1 where ID = 0")) return false;
    else{
        emit sig_RemainingSpotDecreased();
        return true;
    }
}


bool DatabaseManager::isConnected()
{
    return database.isOpen();
}

bool DatabaseManager::SetOtoparkInfo(OtoparkInfo** out_otoparkInfo)
{
    if(*out_otoparkInfo){
         delete *out_otoparkInfo;
        *out_otoparkInfo = nullptr;
    }
    QSqlQuery query;
    if(!query.exec("select fk_CurrentPlanID from OtoparkInfo where ID = 0")) return false;
    if(!query.next()) return false;
    *out_otoparkInfo = new OtoparkInfo(query.value(0).toInt());
    return true;
}

bool DatabaseManager::UpdateCurrentPricingPlan(const qint32& planid, QString& errmsg)
{
    QSqlQuery query;
    query.prepare("update OtoparkInfo set fk_CurrentPlanID = :id");
    query.bindValue(":id",planid);
    if(!query.exec()){
        errmsg = query.lastError().text();
        return false;
    }
    return true;
}

QMap<QString,qint32> DatabaseManager::getColors()
{
    return m_colors;
}

QMap<QString,qint32> DatabaseManager::getVehicleTypes()
{
    return m_vehicleTypes;
}

void DatabaseManager::CreateDatabaseBackup()
{
    QDir().mkpath(m_dbbackuppath);
    QString backupfile = m_dbbackuppath + QDateTime::currentDateTime().toString("dd_MM_yyyy_HH_mm_ss") + ".sqlite";
    QFile::copy(m_dbfile,backupfile);
    QFile(backupfile).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

void DatabaseManager::getColorsFromDB()
{
    QSqlQuery query;
    query.exec("select * from Colors where id != 0");
    while(query.next()){
        qint32 colorID = query.value(0).toInt();
        QString color = query.value(1).toString();
        m_colors[color] = colorID;
    }
}

void DatabaseManager::getVehicleTypesFromDB()
{
    QSqlQuery query;
    query.exec("select * from VehicleTypes where id != 0");
    while(query.next()){
        qint32 typeID = query.value(0).toInt();
        QString type = query.value(1).toString();
        m_vehicleTypes[type] = typeID;
    }
}

