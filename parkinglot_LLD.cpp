#include <optional>
using namespace std;
// ---------------- ENUMS ----------------

enum class VehicleType {
    BIKE,
    CAR,
    TRUCK
};

enum class SpotType {
    BIKE,
    COMPACT,
    LARGE
};

enum class TicketStatus {
    ACTIVE,
    PAID,
    LOST
};

enum class PaymentMode {
    CASH,
    CARD,
    UPI
};

enum class PaymentStatus {
    PENDING,
    SUCCESS,
    FAILED
};


// ---------------- VEHICLE ----------------

class Vehicle {
private:
    string plate;
    VehicleType type;

public:
   Vehicle(string plate, VehicleType type) {
    this->plate = plate;
    this->type = type;
}

    string getPlate() const {
        return plate;
    }

    VehicleType getType() const {
        return type;
    }
};


// ---------------- PARKING SPOT ----------------

class ParkingSpot {
private:
    string id;
    SpotType type;
    optional<Vehicle> parkedVehicle;

public:
   ParkingSpot(string id, SpotType type) {
    this->id = id;
    this->type = type;
    this->parkedVehicle = nullopt;
}
    string getId() const {
        return id;
    }

    SpotType getType() const {
        return type;
    }

    bool isFree() const {
        return !parkedVehicle.has_value();
    }

    optional<Vehicle> getParkedVehicle() const {
        return parkedVehicle;
    }

    void park(const Vehicle& vehicle) {
        if (!isFree()) {
            throw runtime_error("Spot already occupied");
        }

        parkedVehicle = vehicle;
    }

    Vehicle unpark() {

    if (!isFree()) {
        Vehicle v = parkedVehicle.value();

        parkedVehicle.reset();

        return v;
    }

    throw runtime_error("Spot " + id + " is already free");
    }
};


// ---------------- TICKET ----------------

class Ticket {
private:
    string ticketId;
    string vehiclePlate;
    string spotId;
    SpotType assignedSpotType;

    chrono::system_clock::time_point entryTime;
    TicketStatus status;

public:
   Ticket(string id, const Vehicle& vehicle,
       const ParkingSpot& spot) {

    this->ticketId = id;
    this->vehiclePlate = vehicle.getPlate();
    this->spotId = spot.getId();
    this->assignedSpotType = spot.getType();

    this->entryTime = chrono::system_clock::now();
    this->status = TicketStatus::ACTIVE;
}
    string getTicketId() const {
        return ticketId;
    }

    string getVehiclePlate() const {
        return vehiclePlate;
    }

    string getSpotId() const {
        return spotId;
    }

    SpotType getAssignedSpotType() const {
        return assignedSpotType;
    }

    chrono::system_clock::time_point getEntryTime() const {
        return entryTime;
    }

    TicketStatus getStatus() const {
        return status;
    }

    void markAsPaid() {
        if (status != TicketStatus::ACTIVE) {
            throw runtime_error("Ticket is not active");
        }

        status = TicketStatus::PAID;
    }
};


// ---------------- PAYMENT RECEIPT ----------------

class PaymentReceipt {
private:
    string receiptId;
    int amount;
    PaymentMode mode;
    PaymentStatus status;

public:
   PaymentReceipt(string receiptId, int amount,
               PaymentMode mode, PaymentStatus status) {

    this->receiptId = receiptId;
    this->amount = amount;
    this->mode = mode;
    this->status = status;
}

    string getReceiptId() const {
        return receiptId;
    }

    int getAmount() const {
        return amount;
    }

    PaymentMode getMode() const {
        return mode;
    }

    PaymentStatus getStatus() const {
        return status;
    }
};


// Forward declaration
class ParkingLot;


// ---------------- SPOT ALLOCATION STRATEGY ----------------

class SpotAllocationStrategy {
public:
    virtual ParkingSpot* findSpot(
        const Vehicle& vehicle,
        ParkingLot& lot
    ) = 0;

    virtual ~SpotAllocationStrategy() = default;
};


class SimpleSpotAllocationStrategy : public SpotAllocationStrategy {
private:
    SpotType requiredSpot(VehicleType type) {
        switch (type) {
            case VehicleType::BIKE:
                return SpotType::BIKE;

            case VehicleType::CAR:
                return SpotType::COMPACT;

            case VehicleType::TRUCK:
                return SpotType::LARGE;
        }

        throw runtime_error("Invalid vehicle type");
    }

public:
    ParkingSpot* findSpot(
        const Vehicle& vehicle,
        ParkingLot& lot
    ) override;
};


// ---------------- PRICING STRATEGY ----------------

class PricingStrategy {
public:
    virtual int calculateAmount(
        const Ticket& ticket,
        chrono::system_clock::time_point exitTime
    ) = 0;

    virtual ~PricingStrategy() = default;
};


class SpotBasedPricingStrategy : public PricingStrategy {
public:
    int calculateAmount(
        const Ticket& ticket,
        chrono::system_clock::time_point exitTime
    ) override {

        if (exitTime < ticket.getEntryTime()) {
            throw runtime_error("Invalid exit time");
        }

        auto duration = chrono::duration_cast<chrono::minutes>(
            exitTime - ticket.getEntryTime()
        );

        long minutes = duration.count();

        // Round up and charge at least one hour
        long hours = max(1L, (minutes + 59) / 60);

        int ratePerHour = 0;

        switch (ticket.getAssignedSpotType()) {
            case SpotType::BIKE:
                ratePerHour = 10;
                break;

            case SpotType::COMPACT:
                ratePerHour = 30;
                break;

            case SpotType::LARGE:
                ratePerHour = 50;
                break;
        }

        return hours * ratePerHour;
    }
};


// ---------------- PARKING LOT ----------------

class ParkingLot {
private:
    vector<ParkingSpot> spots;
    unordered_map<string, Ticket> tickets;

    unique_ptr<SpotAllocationStrategy> allocationStrategy;
    unique_ptr<PricingStrategy> pricingStrategy;

    int nextTicketId = 1;
    int nextReceiptId = 1;

public:
    ParkingLot(
    unique_ptr<SpotAllocationStrategy> allocation,
    unique_ptr<PricingStrategy> pricing
) {
    this->allocationStrategy = move(allocation);
    this->pricingStrategy = move(pricing);

    this->nextTicketId = 1;
    this->nextReceiptId = 1;
}

    void addSpot(const ParkingSpot& spot) {
        spots.push_back(spot);
    }

    vector<ParkingSpot>& getAllSpots() {
        return spots;
    }

    Ticket parkVehicle(const Vehicle& vehicle) {

        ParkingSpot* spot =
            allocationStrategy->findSpot(vehicle, *this);

        if (spot == nullptr) {
            throw runtime_error("No parking spot available");
        }

        spot->park(vehicle);

        string id = "T" + to_string(nextTicketId++);

        Ticket ticket(id, vehicle, *spot);

        tickets.emplace(id, ticket);

        cout << "Vehicle parked successfully!\n";

        return ticket;
    }

    PaymentReceipt exitVehicle(
        const string& ticketId,
        PaymentMode mode
    ) {

        auto it = tickets.find(ticketId);

        if (it == tickets.end()) {
            throw runtime_error("Invalid ticket");
        }

        Ticket& ticket = it->second;

        if (ticket.getStatus() != TicketStatus::ACTIVE) {
            throw runtime_error("Ticket is not active");
        }

        auto exitTime = chrono::system_clock::now();

        int amount =
            pricingStrategy->calculateAmount(ticket, exitTime);

        cout << "Parking fee: Rs. " << amount << endl;

        // Assume payment is successful
        PaymentStatus paymentStatus = PaymentStatus::SUCCESS;

        // Mark ticket as paid only after successful payment
        if (paymentStatus == PaymentStatus::SUCCESS) {
            ticket.markAsPaid();

            for (ParkingSpot& spot : spots) {
                if (spot.getId() == ticket.getSpotId()) {
                    spot.unpark();
                    break;
                }
            }
        }

        string receiptId = "R" + to_string(nextReceiptId++);

        PaymentReceipt receipt(
            receiptId, amount, mode, paymentStatus
        );

        cout << "Payment successful!\n";
        cout << "Receipt ID: " << receipt.getReceiptId() << endl;

        return receipt;
    }
};


// ---------------- FIND SPOT IMPLEMENTATION ----------------

ParkingSpot* SimpleSpotAllocationStrategy::findSpot(
    const Vehicle& vehicle,
    ParkingLot& lot
) {

    SpotType required = requiredSpot(vehicle.getType());

    for (ParkingSpot& spot : lot.getAllSpots()) {

        if (spot.isFree() && spot.getType() == required) {
            return &spot;
        }
    }

    return nullptr;
}


// ---------------- MAIN ----------------

int main() {

    auto allocation =
        make_unique<SimpleSpotAllocationStrategy>();

    auto pricing =
        make_unique<SpotBasedPricingStrategy>();

    ParkingLot lot(
        move(allocation),
        move(pricing)
    );

    // Add parking spots
    lot.addSpot(ParkingSpot("B1", SpotType::BIKE));
    lot.addSpot(ParkingSpot("C1", SpotType::COMPACT));
    lot.addSpot(ParkingSpot("L1", SpotType::LARGE));

    // Create a car
    Vehicle car("UP65AB1234", VehicleType::CAR);

    // Vehicle enters
    Ticket ticket = lot.parkVehicle(car);

    cout << "Ticket ID: " << ticket.getTicketId() << endl;
    cout << "Vehicle Number: " << ticket.getVehiclePlate() << endl;
    cout << "Spot ID: " << ticket.getSpotId() << endl;

    // Vehicle exits and pays using UPI
    PaymentReceipt receipt =
        lot.exitVehicle(ticket.getTicketId(), PaymentMode::UPI);

    cout << "Amount Paid: Rs. " << receipt.getAmount() << endl;

    return 0;
}
