# AdviseU-Backend

Backend server used for AdviseU's scheduling algorithm

[![CircleCI](https://dl.circleci.com/status-badge/img/circleci/Mt1HHUPfh9bMAZ8BBCzjd3/2UpvYkb8sS1aTRNQkzxmP6/tree/main.svg?style=svg&circle-token=CCIPRJ_W6n7Prv3j4dCBeyVpbnjic_3200b144c5cec74bed326ea73d180fa0f7a7d027)](https://dl.circleci.com/status-badge/redirect/circleci/Mt1HHUPfh9bMAZ8BBCzjd3/2UpvYkb8sS1aTRNQkzxmP6/tree/main)

## Prerequisites

- Docker Desktop
- Docker Compose

## Environment Setup

Before running the application, you need to set up environment variables. Create a `.env` file in the root directory with the variables from `.env.example`.

## Building and Running with Docker

Ensure Docker Desktop is running.

### Using Docker Compose (Recommended)

1. **Build and start the container**:

   ```bash
   docker compose up -d
   ```

2. **View logs**:

   ```bash
   docker compose logs -f
   ```

3. **Stop the container**:

   ```bash
   docker compose down
   ```

4. **Rebuild after code changes**:
   ```bash
   docker compose up -d --build
   ```

### Using Docker Directly

1. **Build the Docker image**:

   ```bash
   docker build -t adviseu-backend .
   ```

2. **Run the container**:

   ```bash
   docker run -p 8080:8080 -e MONGO_URI="mongodb+srv://your_username:your_password@your_cluster.mongodb.net" -d --name adviseu-backend-container adviseu-backend
   ```

3. **View logs**:

   ```bash
   docker logs -f adviseu-backend-container
   ```

4. **Stop and remove the container**:
   ```bash
   docker stop adviseu-backend-container
   docker rm adviseu-backend-container
   ```

## Running Tests

The project includes automated tests using Google Test framework. These tests check what degrees a schedule can be generated for.

**Run all tests using Docker Compose**:

```bash
docker compose --profile test up --build -d && docker compose logs -f test ; docker compose stop
```

**Run a sample of all tests**:

```bash
# Run 1/5th of the tests
docker compose --profile test -e SAMPLING_RATE=5 up --build -d && docker compose logs -f test ; docker compose stop

# Run 1/10th of the tests
docker compose --profile test -e SAMPLING_RATE=10 up --build -d && docker compose logs -f test ; docker compose stop
```

## Testing the API

Once the server is running, you can test the API using the following `curl` command. Ensure the `userId` and `planId` are set to valid program names from the database.

```bash
curl -X POST "http://localhost:8080/api/bypass/generatePlan" \
  -H "Content-Type: application/json" \
  -d '{"userId": "Computer Science Undergraduate Major (BA, BS, HBA, HBS)", "planId": "Computer Systems Option"}'
```

## Development

### Docker Multi-Stage Build

The Dockerfile uses a multi-stage build process to optimize build time and image size:

- Base dependencies are installed in the initial stage
- Individual components (CMake, Boost, Gecode, MongoDB driver) are built in separate stages
- Only the necessary artifacts are copied to the final stage

This approach significantly speeds up rebuilds when you only change application code.

### Environment Variables

The application requires the following environment variables:

- `MONGO_URI`: MongoDB connection string (required)
- `CONTAINER_PORT`: Port the application listens on inside the container (defaults to 8080)

These can be set in the `.env` file or passed directly to the Docker run command.

## Or Build Manually

- First, build/install `Gecode` on your machine from `libs/gecode-release-6.2.0`.

  ```bash
  cd libs/gecode-release-6.2.0
  ./configure --disable-examples --prefix=$(pwd)/install
  make
  make install
  export LD_LIBRARY_PATH=$(pwd)/install/lib:$LD_LIBRARY_PATH
  ```

- Second, build/install `mongo-cxx-driver` on your machine from `libs/mongo-cxx-driver-r4.0.0`.

  ```bash
  cd libs/mongo-cxx-driver-r4.0.0
  mkdir -p build
  cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17
  cmake --build .
  sudo cmake --build . --target install
  ```

- Third, clone/build/install `Boost >=1.88.0` on your machine.

  ```bash
  cd libs
  git clone --recursive https://github.com/boostorg/boost.git
  cd boost
  mkdir -p build
  cd build
  cmake ..
  cmake --build .
  sudo cmake --build . --target install
  ```

- Ensure your MongoDB credentials are added to `src/main.cpp:29`.
- Then you can run the following to build and start the server.
  ```bash
  rm -r build
  mkdir build
  cd build
  cmake ..
  cmake --build .
  ./AdviseU-Backend
  ```
## API Documentation

The backend exposes the following API endpoints to check its status, generate isolated degree plans, and generate user degree plans:

```yaml
openapi: 3.0.3
info:
  title: Plan Generation API
  description: API for health checks and plan generation with user/plan validation
  version: 1.0.0

paths:
  /health:
    get:
      summary: Health check endpoint
      description: Returns the health status of the API
      operationId: healthCheck
      tags:
        - Health
      responses:
        '200':
          description: API is healthy and operational
          content:
            application/json:
              schema:
                type: string
                example: OK

  /api/generatePlan:
    post:
      summary: Generate a plan for a user
      description: Generates a schedule for a specific plan belonging to a user. Updates the plan in MongoDB upon successful completion.
      operationId: generatePlan
      tags:
        - Plan Generation
      requestBody:
        required: true
        content:
          application/json:
            schema:
              type: object
              required:
                - userId
                - planId
              properties:
                userId:
                  type: string
                  description: The MongoDB ObjectId of the user requesting the schedule
                  example: "507f1f77bcf86cd799439011"
                planId:
                  type: string
                  description: The MongoDB ObjectId of the plan to be scheduled
                  example: "507f1f77bcf86cd799439012"
      responses:
        '200':
          description: Schedule was successfully completed
          content:
            application/json:
              schema:
                type: string
                example: "OK"
        '400':
          description: Bad request - Invalid plan/user combination
          content:
            application/json:
              schema:
                type: string
                example: Invalid plan/user combination
        '500':
          description: Internal server error - plan not found
          content:
            application/json:
              schema:
                type: string
                example: Could not find valid plan

  /api/bypass/generatePlan:
    post:
      summary: Generate a plan using major and minor IDs (bypass endpoint)
      description: Bypass API that allows direct input of major and minor IDs without referencing a user or plan. The minor ID is optional.
      operationId: bypassGeneratePlan
      tags:
        - Plan Generation
      requestBody:
        required: true
        content:
          application/json:
            schema:
              type: object
              required:
                - userId
                - planId
              properties:
                userId:
                  type: string
                  description: The MongoDB ObjectId of the major being scheduled
                  example: "507f1f77bcf86cd799439013"
                planId:
                  type: string
                  description: The MongoDB ObjectId of the minor being scheduled (optional)
                  example: "507f1f77bcf86cd799439014"
                  nullable: true
      responses:
        '200':
          description: Schedule was successfully completed
          content:
            application/json:
              schema:
                type: string
                example: "OK"
        '400':
          description: Bad request - Invalid plan/user combination
          content:
            application/json:
              schema:
                type: string
                example: Invalid plan/user combination
        '500':
          description: Internal server error - plan not found
          content:
            application/json:
              schema:
                type: string
                example: Could not find valid plan

tags:
  - name: Health
    description: Health check operations
  - name: Plan Generation
    description: Plan generation and scheduling operations
```
